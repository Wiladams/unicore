// test_unicode_database_script_extensions.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "test_core.h"

#include "unicode_database.h"
#include "unicode_script.h"
#include "unicode_script_set.h"


namespace waavs
{
    // ========================================================================
    // testUnicodeDatabaseScriptExtensions
    //
    // Validate the persisted Script and Script_Extensions runtime database.
    //
    // This test operates on the final .ucdb image rather than rebuilding the
    // UCD inputs. It therefore verifies the complete persistent path:
    //
    //      Scripts.txt / ScriptExtensions.txt
    //              |
    //              v
    //      database generator
    //              |
    //              v
    //      .ucdb serialization
    //              |
    //              v
    //      UnicodeDatabase::reset()
    //              |
    //              +-> script(cp)
    //              |
    //              +-> scriptExtensions(cp)
    //
    // Unicode 17.0.0 expected persistent results:
    //
    //      Script records:              176
    //      Script_Extensions ranges:    206
    //      Explicit code points:        669
    //      Unique Script sets:          118
    //
    // ========================================================================

    static bool testUnicodeDatabaseScriptExtensions(const ByteSpan& source)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "Unicode database Script_Extensions: FAIL: %s\n",
                message);

            return false;
            };


        // ====================================================================
        // Attach runtime database
        // ====================================================================

        UnicodeDatabase database;

        if (!database.reset(source))
            return fail("UnicodeDatabase rejected database image");

        if (!database.valid())
            return fail("runtime database is not valid");


        // ====================================================================
        // Format / Unicode version
        // ====================================================================

        const UnicodeDatabaseHeader* header = database.header();

        if (!header)
            return fail("runtime database has no header");

        if (header->formatMajor != 1 || header->formatMinor != 0)
            return fail("database is not format 1.0");

        if (header->headerSize != kUnicodeDatabaseHeaderSize)
            return fail("database does not have the expected 512-byte header");



        if (database.unicodeMajor() != 17 ||
            database.unicodeMinor() != 0 ||
            database.unicodePatch() != 0)
        {
            return fail("database is not Unicode 17.0.0");
        }


        // ====================================================================
        // Required runtime capabilities
        // ====================================================================

        if (!database.hasScript())
            return fail("dense Script property is missing");

        if (!database.hasScriptExtensions())
            return fail("Script_Extensions data is missing");


        // ====================================================================
        // Expected Unicode 17.0.0 counts
        // ====================================================================

        static constexpr uint32_t kExpectedScriptCount = 176;
        static constexpr uint32_t kExpectedRangeCount = 206;
        static constexpr uint32_t kExpectedSetCount = 118;
        static constexpr uint32_t kExpectedExplicitCodePoints = 669;


        if (database.scriptCount() != kExpectedScriptCount)
            return fail("unexpected Script record count");

        if (database.scriptExtensionRangeCount() != kExpectedRangeCount)
            return fail("unexpected Script_Extensions range count");

        if (database.scriptSetCount() != kExpectedSetCount)
            return fail("unexpected Script set count");


        // ====================================================================
        // Script_Extensions physical structure
        //
        // Validate the public runtime views independently of reset()'s internal
        // validation.
        // ====================================================================

        uint32_t explicitCodePoints = 0;
        uint32_t previousLast = 0;
        bool havePrevious = false;

        std::vector<bool> referencedSets(database.scriptSetCount(), false);


        for (uint32_t i = 0; i < database.scriptExtensionRangeCount(); ++i)
        {
            const UnicodeScriptExtensionRange* range =
                database.scriptExtensionRangeRecord(i);

            if (!range)
                return fail("unable to read Script_Extensions range record");

            if (range->first > range->last)
                return fail("Script_Extensions range has first > last");

            if (range->last >= kUnicodeLimit)
                return fail("Script_Extensions range exceeds Unicode limit");

            if (havePrevious && range->first <= previousLast)
                return fail("Script_Extensions ranges are not strictly ordered");

            if (range->setIndex >= database.scriptSetCount())
                return fail("Script_Extensions range has invalid set index");

            const UnicodeScriptSet* set = database.scriptSet(range->setIndex);

            if (!set)
                return fail("unable to read Script_Extensions set");

            if (set->empty())
                return fail("Script_Extensions range references an empty set");

            referencedSets[range->setIndex] = true;

            explicitCodePoints += range->last - range->first + 1u;

            previousLast = range->last;
            havePrevious = true;
        }


        if (explicitCodePoints != kExpectedExplicitCodePoints)
            return fail("unexpected explicit Script_Extensions code-point count");


        // ====================================================================
        // Every physical set must be referenced.
        // ====================================================================

        for (uint32_t i = 0; i < database.scriptSetCount(); ++i)
        {
            if (!database.scriptSet(i))
                return fail("unable to read Script set");

            if (!referencedSets[i])
                return fail("unreferenced Script set");
        }


        // ====================================================================
        // Physical set deduplication
        // ====================================================================

        for (uint32_t i = 0; i < database.scriptSetCount(); ++i)
        {
            const UnicodeScriptSet* a = database.scriptSet(i);

            for (uint32_t j = 0; j < i; ++j)
            {
                const UnicodeScriptSet* b = database.scriptSet(j);

                if (*a == *b)
                    return fail("duplicate physical Script sets");
            }
        }


        // ====================================================================
        // Dense Script table
        //
        // Every Unicode code point must resolve to a valid Script index, and
        // the corresponding Script coverage must contain that code point.
        // ====================================================================

        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            const UnicodeScriptIndex scriptIndex = database.script(cp);

            if (scriptIndex == kUnicodeScriptIndexInvalid)
            {
                std::printf(
                    "Unicode database Script_Extensions: FAIL: invalid Script\n"
                    "  Code point: U+%04X\n",
                    cp);

                return false;
            }

            if (scriptIndex >= database.scriptCount())
            {
                std::printf(
                    "Unicode database Script_Extensions: FAIL: Script index out of range\n"
                    "  Code point: U+%04X\n"
                    "  Script:     %u\n",
                    cp,
                    static_cast<unsigned>(scriptIndex));

                return false;
            }

            if (!database.scriptCoverage(scriptIndex).contains(cp))
            {
                std::printf(
                    "Unicode database Script_Extensions: FAIL: Script coverage mismatch\n"
                    "  Code point: U+%04X\n"
                    "  Script:     %u\n",
                    cp,
                    static_cast<unsigned>(scriptIndex));

                return false;
            }
        }


        // ====================================================================
        // Script_Extensions semantic lookup
        //
        // Walk the complete Unicode address space while walking the sparse
        // explicit range array in parallel.
        //
        // For an explicitly represented code point:
        //
        //      scriptExtensions(cp) == stored Script set
        //
        // Otherwise:
        //
        //      scriptExtensions(cp) == { script(cp) }
        //
        // This validates both sides of the runtime lookup contract.
        // ====================================================================

        uint32_t rangeIndex = 0;
        uint32_t explicitLookupCount = 0;
        uint32_t fallbackLookupCount = 0;

        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            while (rangeIndex < database.scriptExtensionRangeCount())
            {
                const UnicodeScriptExtensionRange* range =
                    database.scriptExtensionRangeRecord(rangeIndex);

                if (!range)
                    return fail("unable to read range during lookup validation");

                if (cp <= range->last)
                    break;

                ++rangeIndex;
            }


            const UnicodeScriptExtensionRange* range = nullptr;

            if (rangeIndex < database.scriptExtensionRangeCount())
            {
                const UnicodeScriptExtensionRange* candidate =
                    database.scriptExtensionRangeRecord(rangeIndex);

                if (candidate && cp >= candidate->first && cp <= candidate->last)
                    range = candidate;
            }


            UnicodeScriptSet expected{};

            if (range)
            {
                const UnicodeScriptSet* storedSet = database.scriptSet(range->setIndex);

                if (!storedSet)
                    return fail("explicit range references missing Script set");

                expected = *storedSet;
                ++explicitLookupCount;
            }
            else
            {
                expected = UnicodeScriptSet::singleton(database.script(cp));
                ++fallbackLookupCount;
            }


            const UnicodeScriptSet actual = database.scriptExtensions(cp);

            if (actual != expected)
            {
                std::printf(
                    "Unicode database Script_Extensions: FAIL: lookup mismatch\n"
                    "  Code point: U+%04X\n"
                    "  Explicit:   %s\n",
                    cp,
                    range ? "YES" : "NO");

                return false;
            }
        }


        if (explicitLookupCount != kExpectedExplicitCodePoints)
            return fail("explicit Script_Extensions lookup count mismatch");

        if (explicitLookupCount + fallbackLookupCount != kUnicodeLimit)
            return fail("Script_Extensions lookup did not cover Unicode space");


        // ====================================================================
        // Locate Script indices by ISO 15924 tag
        //
        // Do not hard-code persistent Script indices. Their numeric values are
        // an internal consequence of database construction order.
        // ====================================================================

        auto findScript = [&](const char* iso15924) -> UnicodeScriptIndex {
            for (uint32_t i = 0; i < database.scriptCount(); ++i)
            {
                InternedKey tag = database.scriptISO15924(i);

                if (tag && std::strcmp(tag, iso15924) == 0)
                    return static_cast<UnicodeScriptIndex>(i);
            }

            return kUnicodeScriptIndexInvalid;
            };


        const UnicodeScriptIndex latin = findScript("Latn");
        const UnicodeScriptIndex greek = findScript("Grek");
        const UnicodeScriptIndex arabic = findScript("Arab");
        const UnicodeScriptIndex telugu = findScript("Telu");
        const UnicodeScriptIndex common = findScript("Zyyy");
        const UnicodeScriptIndex inherited = findScript("Zinh");
        const UnicodeScriptIndex unknown = findScript("Zzzz");


        if (latin == kUnicodeScriptIndexInvalid ||
            greek == kUnicodeScriptIndexInvalid ||
            arabic == kUnicodeScriptIndexInvalid ||
            telugu == kUnicodeScriptIndexInvalid ||
            common == kUnicodeScriptIndexInvalid ||
            inherited == kUnicodeScriptIndexInvalid ||
            unknown == kUnicodeScriptIndexInvalid)
        {
            return fail("unable to locate expected Script records");
        }


        // ====================================================================
        // Familiar dense Script spot checks
        // ====================================================================

        struct ScriptCheck
        {
            uint32_t cp;
            UnicodeScriptIndex expected;
            const char* description;
        };


        const ScriptCheck checks[] =
        {
            { 0x0041, latin,     "Latin capital A" },
            { 0x03B1, greek,     "Greek small alpha" },
            { 0x0627, arabic,    "Arabic alef" },
            { 0x0C15, telugu,    "Telugu KA" },
            { 0x0020, common,    "space" },
            { 0x0301, inherited, "combining acute accent" },
            { 0x0378, unknown,   "unassigned code point" }
        };


        for (const ScriptCheck& check : checks)
        {
            const UnicodeScriptIndex actual = database.script(check.cp);

            if (actual != check.expected)
            {
                std::printf(
                    "Unicode database Script_Extensions: FAIL: Script spot check\n"
                    "  Code point: U+%04X\n"
                    "  Name:       %s\n"
                    "  Expected:   %u\n"
                    "  Actual:     %u\n",
                    check.cp,
                    check.description,
                    static_cast<unsigned>(check.expected),
                    static_cast<unsigned>(actual));

                return false;
            }
        }


        // ====================================================================
        // Out-of-range behavior
        // ====================================================================

        if (database.script(kUnicodeLimit) != kUnicodeScriptIndexInvalid)
            return fail("out-of-range Script lookup did not return invalid");

        if (!database.scriptExtensions(kUnicodeLimit).empty())
            return fail("out-of-range Script_Extensions lookup did not return empty set");


        // ====================================================================
        // Diagnostics
        // ====================================================================

        std::printf(
            "Unicode database Script_Extensions: PASS\n"
            "  Script records:          %u\n"
            "  Explicit ranges:         %u\n"
            "  Explicit code points:    %u\n"
            "  Unique Script sets:      %u\n"
            "  Dense Script coverage:   PASS\n"
            "  Explicit lookup:         PASS\n"
            "  Fallback lookup:         PASS\n",
            database.scriptCount(),
            database.scriptExtensionRangeCount(),
            explicitLookupCount,
            database.scriptSetCount());


        return true;
    }


    // ========================================================================
    // Convenience filename overload
    // ========================================================================

    static bool testUnicodeDatabaseScriptExtensions(const char* filename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(filename, fileData))
        {
            std::printf(
                "Unicode database Script_Extensions: FAIL: unable to read file\n"
                "  File: %s\n",
                filename);

            return false;
        }

        const ByteSpan source(fileData.data(), fileData.size());

        return testUnicodeDatabaseScriptExtensions(source);
    }

} // namespace waavs