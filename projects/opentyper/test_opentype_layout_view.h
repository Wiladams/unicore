// test_opentype_layout_view.h

#pragma once

#include "../ucdbdemo/test_core.h"

#include <cstdint>
#include <cstdio>
#include <vector>

#include "opentype_layout_view.h"


namespace waavs
{
    static void appendLayoutTestU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void appendLayoutTestU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(static_cast<uint8_t>(value >> 24));
        data.push_back(static_cast<uint8_t>(value >> 16));
        data.push_back(static_cast<uint8_t>(value >> 8));
        data.push_back(static_cast<uint8_t>(value));
    }


    static void patchLayoutTestU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
    {
        data[offset] = static_cast<uint8_t>(value >> 8);
        data[offset + 1] = static_cast<uint8_t>(value);
    }


    static std::vector<uint8_t> makeOpenTypeLayoutViewTestData()
    {
        std::vector<uint8_t> data;

        appendLayoutTestU16(data, 1);
        appendLayoutTestU16(data, 0);

        const size_t scriptListPatch = data.size();
        appendLayoutTestU16(data, 0);

        const size_t featureListPatch = data.size();
        appendLayoutTestU16(data, 0);

        const size_t lookupListPatch = data.size();
        appendLayoutTestU16(data, 0);


        const size_t scriptListOffset = data.size();

        appendLayoutTestU16(data, 2);

        appendLayoutTestU32(data, OTAG("DFLT"));
        const size_t dfltScriptPatch = data.size();
        appendLayoutTestU16(data, 0);

        appendLayoutTestU32(data, OTAG("latn"));
        const size_t latnScriptPatch = data.size();
        appendLayoutTestU16(data, 0);


        const size_t dfltScriptOffset = data.size();
        patchLayoutTestU16(data, dfltScriptPatch, static_cast<uint16_t>(dfltScriptOffset - scriptListOffset));

        const size_t dfltDefaultPatch = data.size();
        appendLayoutTestU16(data, 0);
        appendLayoutTestU16(data, 0);

        const size_t dfltDefaultOffset = data.size();
        patchLayoutTestU16(data, dfltDefaultPatch, static_cast<uint16_t>(dfltDefaultOffset - dfltScriptOffset));

        appendLayoutTestU16(data, 0);
        appendLayoutTestU16(data, 0xFFFF);
        appendLayoutTestU16(data, 1);
        appendLayoutTestU16(data, 3);


        const size_t latnScriptOffset = data.size();
        patchLayoutTestU16(data, latnScriptPatch, static_cast<uint16_t>(latnScriptOffset - scriptListOffset));

        const size_t latnDefaultPatch = data.size();
        appendLayoutTestU16(data, 0);

        appendLayoutTestU16(data, 1);

        appendLayoutTestU32(data, OTAG("TRK "));
        const size_t turkishPatch = data.size();
        appendLayoutTestU16(data, 0);


        const size_t latnDefaultOffset = data.size();
        patchLayoutTestU16(data, latnDefaultPatch, static_cast<uint16_t>(latnDefaultOffset - latnScriptOffset));

        appendLayoutTestU16(data, 0);
        appendLayoutTestU16(data, 0xFFFF);
        appendLayoutTestU16(data, 3);
        appendLayoutTestU16(data, 5);
        appendLayoutTestU16(data, 2);
        appendLayoutTestU16(data, 7);


        const size_t turkishOffset = data.size();
        patchLayoutTestU16(data, turkishPatch, static_cast<uint16_t>(turkishOffset - latnScriptOffset));

        appendLayoutTestU16(data, 0);
        appendLayoutTestU16(data, 4);
        appendLayoutTestU16(data, 2);
        appendLayoutTestU16(data, 4);
        appendLayoutTestU16(data, 9);


        const size_t featureListOffset = data.size();
        appendLayoutTestU16(data, 0);

        const size_t lookupListOffset = data.size();
        appendLayoutTestU16(data, 0);


        patchLayoutTestU16(data, scriptListPatch, static_cast<uint16_t>(scriptListOffset));
        patchLayoutTestU16(data, featureListPatch, static_cast<uint16_t>(featureListOffset));
        patchLayoutTestU16(data, lookupListPatch, static_cast<uint16_t>(lookupListOffset));

        return data;
    }


    static bool testOpenTypeLayoutView()
    {
        uint32_t cases = 0;
        uint32_t passed = 0;

        auto fail = [&](const char* message) -> bool
            {
                std::printf("OpenType layout views: FAIL\n  %s\n", message);
                return false;
            };


        // ====================================================================
        // Case 1
        //
        // Top-level layout and ScriptList.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeLayoutViewTestData();
            OpenTypeLayoutView layout(ByteSpan(data.data(), data.size()));

            if (!layout)
                return fail("case 1 layout invalid");

            const OpenTypeLayoutScriptListView scripts = layout.scripts();

            if (!scripts || scripts.size() != 2)
                return fail("case 1 ScriptList");

            if (!scripts.contains(OTAG("DFLT")) || !scripts.contains(OTAG("latn")))
                return fail("case 1 expected scripts missing");

            if (scripts.contains(OTAG("arab")))
                return fail("case 1 unexpected script");

            ++passed;
        }


        // ====================================================================
        // Case 2
        //
        // DFLT default LangSys.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeLayoutViewTestData();
            OpenTypeLayoutView layout(ByteSpan(data.data(), data.size()));

            const OpenTypeLayoutScriptView script = layout.script(OTAG("DFLT"));

            if (!script || !script.hasDefaultLangSys())
                return fail("case 2 DFLT script");

            const OpenTypeLayoutLangSysView langSys = script.defaultLangSys();

            if (!langSys)
                return fail("case 2 default LangSys");

            if (langSys.requiredFeatureIndex() != kOpenTypeNoRequiredFeature)
                return fail("case 2 unexpected required feature");

            if (langSys.featureCount() != 1)
                return fail("case 2 feature count");

            uint16_t featureIndex = 0;

            if (!langSys.featureIndex(0, featureIndex) || featureIndex != 3)
                return fail("case 2 feature index");

            ++passed;
        }


        // ====================================================================
        // Case 3
        //
        // latn default LangSys.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeLayoutViewTestData();
            OpenTypeLayoutView layout(ByteSpan(data.data(), data.size()));

            const OpenTypeLayoutLangSysView langSys = layout.script(OTAG("latn")).defaultLangSys();

            if (!langSys || langSys.featureCount() != 3)
                return fail("case 3 Latin default LangSys");

            const uint16_t expected[] = { 5, 2, 7 };

            for (size_t i = 0; i < 3; ++i)
            {
                uint16_t featureIndex = 0;

                if (!langSys.featureIndex(i, featureIndex) || featureIndex != expected[i])
                    return fail("case 3 feature indices");
            }

            ++passed;
        }


        // ====================================================================
        // Case 4
        //
        // Named language system.
        // ====================================================================

        {
            ++cases;

            const std::vector<uint8_t> data = makeOpenTypeLayoutViewTestData();
            OpenTypeLayoutView layout(ByteSpan(data.data(), data.size()));

            const OpenTypeLayoutScriptView script = layout.script(OTAG("latn"));

            if (!script || script.languageCount() != 1)
                return fail("case 4 language count");

            Tag languageTag = 0;

            if (!script.languageTag(0, languageTag) || languageTag != OTAG("TRK "))
                return fail("case 4 language tag");

            const OpenTypeLayoutLangSysView langSys = script.findLanguage(OTAG("TRK "));

            if (!langSys)
                return fail("case 4 Turkish LangSys");

            if (langSys.requiredFeatureIndex() != 4 || langSys.featureCount() != 2)
                return fail("case 4 Turkish feature metadata");

            uint16_t feature0 = 0;
            uint16_t feature1 = 0;

            if (!langSys.featureIndex(0, feature0) || !langSys.featureIndex(1, feature1))
                return fail("case 4 Turkish feature indices");

            if (feature0 != 4 || feature1 != 9)
                return fail("case 4 wrong Turkish features");

            ++passed;
        }


        // ====================================================================
        // Case 5
        //
        // Lazy traversal.
        //
        // Corrupt DFLT's Script offset. Latin remains queryable because the
        // DFLT Script table is never dereferenced while looking for latn.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> data = makeOpenTypeLayoutViewTestData();

            const size_t scriptListOffset = 10;
            const size_t dfltScriptOffsetField = scriptListOffset + 6;

            patchLayoutTestU16(data, dfltScriptOffsetField, 0xFFFF);

            OpenTypeLayoutView layout(ByteSpan(data.data(), data.size()));

            if (!layout)
                return fail("case 5 top-level layout rejected lazy corruption");

            if (!layout.hasScript(OTAG("DFLT")) || !layout.hasScript(OTAG("latn")))
                return fail("case 5 ScriptRecord query failed");

            if (layout.script(OTAG("DFLT")))
                return fail("case 5 malformed DFLT Script accepted");

            if (!layout.script(OTAG("latn")))
                return fail("case 5 unrelated Latin Script damaged");

            ++passed;
        }


        // ====================================================================
        // Case 6
        //
        // Invalid top-level layout headers are rejected.
        // ====================================================================

        {
            ++cases;

            std::vector<uint8_t> badVersion = makeOpenTypeLayoutViewTestData();
            badVersion[1] = 2;

            OpenTypeLayoutView badVersionLayout(ByteSpan(badVersion.data(), badVersion.size()));

            if (badVersionLayout)
                return fail("case 6 unsupported version accepted");

            const uint8_t truncatedBytes[] = { 0x00, 0x01, 0x00, 0x00, 0x00 };
            OpenTypeLayoutView truncated(ByteSpan(truncatedBytes, sizeof(truncatedBytes)));

            if (truncated)
                return fail("case 6 truncated header accepted");

            ++passed;
        }


        std::printf(
            "OpenType layout views: PASS\n"
            "  Cases:                 %u\n"
            "  Passed:                %u\n"
            "  ScriptList:            PASS\n"
            "  Default LangSys:       PASS\n"
            "  Named LangSys:         PASS\n"
            "  Feature indices:       PASS\n"
            "  Lazy traversal:        PASS\n"
            "  Failure paths:         PASS\n",
            cases, passed);

        return passed == cases;
    }

} // namespace waavs
