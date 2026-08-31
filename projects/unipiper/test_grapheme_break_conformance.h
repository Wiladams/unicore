// test_grapheme_break_conformance.h

#pragma once



#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include "unicode_database.h"
#include "unicode_grapheme_property_stream.h"
#include "unicode_grapheme_stream.h"
#include "unicode_scalar_stream.h"


namespace waavs
{
    // ========================================================================
    // GraphemeBreakScalarSource
    //
    // Supplies UnicodeScalar values directly from the code points listed in
    // GraphemeBreakTest.txt.
    //
    // Source ranges are synthetic scalar-index ranges:
    //
    //      scalar 0 -> [0,1)
    //      scalar 1 -> [1,2)
    //      ...
    //
    // This makes GraphemeClusterView::source directly comparable with the
    // expected scalar boundaries.
    // ========================================================================

    class GraphemeBreakScalarSource
    {
    public:
        GraphemeBreakScalarSource(const uint32_t* codePoints, size_t count) noexcept
            : mCodePoints(codePoints)
            , mCount(count)
        {
            if (count == 0)
                mStatus = TextStreamStatus::End;
        }


        bool operator()(UnicodeScalar& out) noexcept
        {
            if (mStatus != TextStreamStatus::Ready)
                return false;


            if (mIndex >= mCount)
            {
                mStatus = TextStreamStatus::End;
                return false;
            }


            UnicodeScalar result;

            result.value =
                mCodePoints[mIndex];

            result.source.begin =
                static_cast<TextOffset>(mIndex);

            result.source.end =
                static_cast<TextOffset>(mIndex + 1);


            ++mIndex;

            out = result;

            return true;
        }


        [[nodiscard]]
        TextStreamStatus status() const noexcept {
            return mStatus;
        }


    private:
        const uint32_t* mCodePoints{ nullptr };

        size_t mCount{ 0 };
        size_t mIndex{ 0 };

        TextStreamStatus mStatus{ TextStreamStatus::Ready };
    };


    // ========================================================================
    // GraphemeBreakTest parser
    //
    // The official file uses two UTF-8 marker characters:
    //
    //      break marker:
    //          U+00F7
    //          UTF-8 C3 B7
    //
    //      no-break marker:
    //          U+00D7
    //          UTF-8 C3 97
    //
    // Keep the C++ source itself ASCII-only by recognizing their UTF-8 bytes.
    // ========================================================================

    enum class GraphemeBreakLineResult : uint8_t
    {
        Skip = 0,
        Parsed,
        Error
    };


    static bool graphemeBreakIsSpace(uint8_t ch) noexcept
    {
        return
            ch == ' ' ||
            ch == '\t' ||
            ch == '\r';
    }


    static void graphemeBreakSkipSpaces(
        const uint8_t*& current,
        const uint8_t* end) noexcept
    {
        while (current < end &&
            graphemeBreakIsSpace(*current))
        {
            ++current;
        }
    }


    static bool graphemeBreakHexDigit(
        uint8_t ch,
        uint32_t& value) noexcept
    {
        if (ch >= '0' && ch <= '9')
        {
            value = ch - '0';
            return true;
        }


        if (ch >= 'A' && ch <= 'F')
        {
            value = ch - 'A' + 10u;
            return true;
        }


        if (ch >= 'a' && ch <= 'f')
        {
            value = ch - 'a' + 10u;
            return true;
        }


        return false;
    }


    static bool graphemeBreakReadCodePoint(
        const uint8_t*& current,
        const uint8_t* end,
        uint32_t& outCodePoint) noexcept
    {
        uint32_t value = 0;
        uint32_t digits = 0;


        while (current < end)
        {
            uint32_t digit = 0;

            if (!graphemeBreakHexDigit(
                *current,
                digit))
            {
                break;
            }


            if (digits >= 6)
                return false;


            value =
                (value << 4) |
                digit;

            ++digits;
            ++current;
        }


        if (digits == 0)
            return false;


        if (value >= 0x110000u)
            return false;


        if (value >= 0xD800u &&
            value <= 0xDFFFu)
        {
            return false;
        }


        outCodePoint = value;

        return true;
    }


    static bool graphemeBreakReadMarker(
        const uint8_t*& current,
        const uint8_t* end,
        bool& outBreak) noexcept
    {
        if (end - current < 2)
            return false;


        // U+00F7, UTF-8 C3 B7.
        if (current[0] == 0xC3 &&
            current[1] == 0xB7)
        {
            current += 2;

            outBreak = true;

            return true;
        }


        // U+00D7, UTF-8 C3 97.
        if (current[0] == 0xC3 &&
            current[1] == 0x97)
        {
            current += 2;

            outBreak = false;

            return true;
        }


        return false;
    }


    static GraphemeBreakLineResult graphemeBreakParseLine(
        const uint8_t* begin,
        const uint8_t* end,
        std::vector<uint32_t>& codePoints,
        std::vector<uint32_t>& breaks)
    {
        codePoints.clear();
        breaks.clear();


        const uint8_t* current =
            begin;


        // Optional UTF-8 BOM.
        if (end - current >= 3 &&
            current[0] == 0xEF &&
            current[1] == 0xBB &&
            current[2] == 0xBF)
        {
            current += 3;
        }


        graphemeBreakSkipSpaces(
            current,
            end);


        if (current == end ||
            *current == '#')
        {
            return GraphemeBreakLineResult::Skip;
        }


        bool expectMarker = true;


        while (current < end)
        {
            graphemeBreakSkipSpaces(
                current,
                end);


            if (current == end ||
                *current == '#')
            {
                break;
            }


            if (expectMarker)
            {
                bool isBreak = false;


                if (!graphemeBreakReadMarker(
                    current,
                    end,
                    isBreak))
                {
                    return GraphemeBreakLineResult::Error;
                }


                if (isBreak)
                {
                    breaks.push_back(
                        static_cast<uint32_t>(
                            codePoints.size()));
                }


                expectMarker = false;
            }
            else
            {
                uint32_t cp = 0;


                if (!graphemeBreakReadCodePoint(
                    current,
                    end,
                    cp))
                {
                    return GraphemeBreakLineResult::Error;
                }


                codePoints.push_back(cp);

                expectMarker = true;
            }
        }


        // A valid test line ends with a marker, so after reading it we are
        // expecting a code point rather than another marker.

        if (expectMarker)
            return GraphemeBreakLineResult::Error;


        if (codePoints.empty())
            return GraphemeBreakLineResult::Error;


        // GraphemeBreakTest lines begin and end with break boundaries.

        if (breaks.empty() ||
            breaks.front() != 0u ||
            breaks.back() != codePoints.size())
        {
            return GraphemeBreakLineResult::Error;
        }


        return GraphemeBreakLineResult::Parsed;
    }


    // ========================================================================
    // Diagnostics
    // ========================================================================

    static void graphemeBreakPrintCodePoints(
        const std::vector<uint32_t>& codePoints)
    {
        std::printf("    code points:");


        for (uint32_t cp : codePoints)
        {
            std::printf(
                " U+%04X",
                cp);
        }


        std::printf("\n");
    }


    static void graphemeBreakPrintPattern(
        const char* label,
        const std::vector<uint32_t>& codePoints,
        const std::vector<uint32_t>& breaks)
    {
        std::printf(
            "    %-9s ",
            label);


        size_t breakIndex = 0;


        for (uint32_t position = 0;
            position <= codePoints.size();
            ++position)
        {
            const bool isBreak =
                breakIndex < breaks.size() &&
                breaks[breakIndex] == position;


            std::printf(
                "%c",
                isBreak ? '|' : '.');


            if (isBreak)
                ++breakIndex;


            if (position < codePoints.size())
            {
                std::printf(
                    " %04X ",
                    codePoints[position]);
            }
        }


        std::printf("\n");
    }


    // ========================================================================
    // Run one GraphemeBreakTest case
    //
    // Pipeline:
    //
    //      listed code points
    //          ->
    //      UnicodeScalar
    //          ->
    //      GraphemePropertyStream
    //          ->
    //      GraphemeStream
    //
    // NFC is deliberately not used here.
    // ========================================================================

    static bool graphemeBreakRunCase(
        const UnicodeDatabase& database,
        const std::vector<uint32_t>& codePoints,
        std::vector<uint32_t>& actualBreaks)
    {
        actualBreaks.clear();

        actualBreaks.push_back(0u);


        GraphemeBreakScalarSource source(
            codePoints.data(),
            codePoints.size());


        GraphemePropertyStream<
            GraphemeBreakScalarSource>
            properties(
                source,
                database);


        if (!properties.valid())
            return false;


        GraphemeStream<
            GraphemePropertyStream<
            GraphemeBreakScalarSource>>
            graphemes(properties);


        GraphemeClusterView cluster;

        uint32_t scalarPosition = 0;


        while (graphemes(cluster))
        {
            if (cluster.scalarCount == 0)
                return false;


            if (cluster.normalizedBegin !=
                scalarPosition)
            {
                return false;
            }


            const uint32_t clusterEnd =
                scalarPosition +
                cluster.scalarCount;


            if (clusterEnd >
                codePoints.size())
            {
                return false;
            }


            // ---------------------------------------------------------------
            // Verify that segmentation did not alter scalar values.
            // ---------------------------------------------------------------

            for (uint32_t i = 0;
                i < cluster.scalarCount;
                ++i)
            {
                if (cluster.scalars[i].value !=
                    codePoints[scalarPosition + i])
                {
                    return false;
                }
            }


            // ---------------------------------------------------------------
            // Synthetic source ranges are scalar-index ranges, so the cluster
            // source envelope should exactly match its scalar span.
            // ---------------------------------------------------------------

            if (cluster.source.begin !=
                scalarPosition ||
                cluster.source.end !=
                clusterEnd)
            {
                return false;
            }


            scalarPosition =
                clusterEnd;


            actualBreaks.push_back(
                scalarPosition);
        }


        if (graphemes.status() !=
            TextStreamStatus::End)
        {
            return false;
        }


        if (properties.status() !=
            TextStreamStatus::End)
        {
            return false;
        }


        if (source.status() !=
            TextStreamStatus::End)
        {
            return false;
        }


        if (scalarPosition !=
            codePoints.size())
        {
            return false;
        }


        return true;
    }


    // ========================================================================
    // Main conformance test
    // ========================================================================

    static bool testGraphemeBreakConformance(
        const ByteSpan& breakTestData,
        const UnicodeDatabase& database)
    {
        if (!database.valid() ||
            !database.hasGraphemeClusterBreak() ||
            !database.hasIndicConjunctBreak() ||
            !database.hasExtendedPictographic())
        {
            std::printf(
                "GraphemeBreakTest: FAIL - database lacks grapheme properties\n");

            return false;
        }


        const uint8_t* current =
            breakTestData.begin();

        const uint8_t* end =
            breakTestData.end();


        uint32_t lineNumber = 0;

        uint32_t testCount = 0;
        uint32_t passCount = 0;
        uint32_t failureCount = 0;

        uint64_t scalarCount = 0;
        uint64_t clusterCount = 0;

        uint32_t maxScalarsPerTest = 0;
        uint32_t maxClusterLength = 0;


        static constexpr uint32_t kMaximumPrintedFailures = 20;


        std::vector<uint32_t> codePoints;
        std::vector<uint32_t> expectedBreaks;
        std::vector<uint32_t> actualBreaks;


        while (current < end)
        {
            const uint8_t* lineBegin =
                current;


            while (current < end &&
                *current != '\n')
            {
                ++current;
            }


            const uint8_t* lineEnd =
                current;


            if (current < end)
                ++current;


            ++lineNumber;


            if (lineEnd > lineBegin &&
                lineEnd[-1] == '\r')
            {
                --lineEnd;
            }


            const GraphemeBreakLineResult parseResult =
                graphemeBreakParseLine(
                    lineBegin,
                    lineEnd,
                    codePoints,
                    expectedBreaks);


            if (parseResult ==
                GraphemeBreakLineResult::Skip)
            {
                continue;
            }


            if (parseResult ==
                GraphemeBreakLineResult::Error)
            {
                std::printf(
                    "GraphemeBreakTest: FAIL - parse error at line %u\n",
                    lineNumber);

                return false;
            }


            ++testCount;

            scalarCount +=
                codePoints.size();

            clusterCount +=
                expectedBreaks.size() - 1u;


            if (codePoints.size() >
                maxScalarsPerTest)
            {
                maxScalarsPerTest =
                    static_cast<uint32_t>(
                        codePoints.size());
            }


            for (size_t i = 1;
                i < expectedBreaks.size();
                ++i)
            {
                const uint32_t length =
                    expectedBreaks[i] -
                    expectedBreaks[i - 1];


                if (length >
                    maxClusterLength)
                {
                    maxClusterLength =
                        length;
                }
            }


            const bool ran =
                graphemeBreakRunCase(
                    database,
                    codePoints,
                    actualBreaks);


            const bool matches =
                ran &&
                actualBreaks == expectedBreaks;


            if (matches)
            {
                ++passCount;
                continue;
            }


            ++failureCount;


            if (failureCount <=
                kMaximumPrintedFailures)
            {
                std::printf(
                    "GraphemeBreakTest: FAIL line %u\n",
                    lineNumber);


                graphemeBreakPrintCodePoints(
                    codePoints);


                graphemeBreakPrintPattern(
                    "expected:",
                    codePoints,
                    expectedBreaks);


                if (ran)
                {
                    graphemeBreakPrintPattern(
                        "actual:",
                        codePoints,
                        actualBreaks);
                }
                else
                {
                    std::printf(
                        "    actual:    stream execution failure\n");
                }
            }
        }


        if (testCount == 0)
        {
            std::printf(
                "GraphemeBreakTest: FAIL - no test cases found\n");

            return false;
        }


        if (failureCount != 0)
        {
            std::printf(
                "GraphemeBreakTest: FAIL\n");

            std::printf(
                "  Tests:                  %u\n",
                testCount);

            std::printf(
                "  Passed:                 %u\n",
                passCount);

            std::printf(
                "  Failed:                 %u\n",
                failureCount);


            if (failureCount >
                kMaximumPrintedFailures)
            {
                std::printf(
                    "  Failure details shown:  %u of %u\n",
                    kMaximumPrintedFailures,
                    failureCount);
            }


            return false;
        }


        std::printf(
            "GraphemeBreakTest: PASS\n");

        std::printf(
            "  Tests:                  %u\n",
            testCount);

        std::printf(
            "  Passed:                 %u\n",
            passCount);

        std::printf(
            "  Scalars tested:         %llu\n",
            static_cast<unsigned long long>(
                scalarCount));

        std::printf(
            "  Expected clusters:      %llu\n",
            static_cast<unsigned long long>(
                clusterCount));

        std::printf(
            "  Maximum test length:    %u\n",
            maxScalarsPerTest);

        std::printf(
            "  Maximum cluster length: %u\n",
            maxClusterLength);


        return true;
    }


    // ========================================================================
    // File helper
    // ========================================================================

    static bool graphemeBreakReadFile(
        const char* filename,
        std::vector<uint8_t>& bytes)
    {
        bytes.clear();


        std::ifstream input(
            filename,
            std::ios::binary |
            std::ios::ate);


        if (!input)
            return false;


        const std::streampos end =
            input.tellg();


        if (end <= 0)
            return false;


        bytes.resize(
            static_cast<size_t>(end));


        input.seekg(
            0,
            std::ios::beg);


        input.read(
            reinterpret_cast<char*>(
                bytes.data()),
            static_cast<std::streamsize>(
                bytes.size()));


        return
            static_cast<bool>(input);
    }


    // ========================================================================
    // Convenience overload
    //
    // Database is already loaded.
    // ========================================================================

    static bool testGraphemeBreakConformance(
        const char* breakTestFilename,
        const UnicodeDatabase& database)
    {
        std::vector<uint8_t> testBytes;


        if (!graphemeBreakReadFile(
            breakTestFilename,
            testBytes))
        {
            std::printf(
                "GraphemeBreakTest: FAIL - unable to read %s\n",
                breakTestFilename);

            return false;
        }


        return testGraphemeBreakConformance(
            ByteSpan(
                testBytes.data(),
                testBytes.size()),
            database);
    }


    // ========================================================================
    // Convenience overload
    //
    // Load both GraphemeBreakTest.txt and the Unicode database.
    // ========================================================================

    static bool testGraphemeBreakConformance(
        const char* breakTestFilename,
        const char* databaseFilename)
    {
        std::vector<uint8_t> testBytes;
        std::vector<uint8_t> databaseBytes;


        if (!graphemeBreakReadFile(
            breakTestFilename,
            testBytes))
        {
            std::printf(
                "GraphemeBreakTest: FAIL - unable to read %s\n",
                breakTestFilename);

            return false;
        }


        if (!graphemeBreakReadFile(
            databaseFilename,
            databaseBytes))
        {
            std::printf(
                "GraphemeBreakTest: FAIL - unable to read %s\n",
                databaseFilename);

            return false;
        }


        UnicodeDatabase database(
            ByteSpan(
                databaseBytes.data(),
                databaseBytes.size()));


        if (!database)
        {
            std::printf(
                "GraphemeBreakTest: FAIL - invalid Unicode database\n");

            return false;
        }


        return testGraphemeBreakConformance(
            ByteSpan(
                testBytes.data(),
                testBytes.size()),
            database);
    }

} // namespace waavs