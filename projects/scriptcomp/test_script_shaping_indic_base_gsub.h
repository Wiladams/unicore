// test_script_shaping_indic_base_gsub.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "script_shaping_ir_executor.h"

namespace waavs
{
    static inline void indicBaseTestPushU16(std::vector<uint8_t>& data, uint16_t value)
    {
        data.push_back(uint8_t(value >> 8));
        data.push_back(uint8_t(value));
    }


    static inline void indicBaseTestPushU32(std::vector<uint8_t>& data, uint32_t value)
    {
        data.push_back(uint8_t(value >> 24));
        data.push_back(uint8_t(value >> 16));
        data.push_back(uint8_t(value >> 8));
        data.push_back(uint8_t(value));
    }


    // ========================================================================
    // Synthetic GSUB
    //
    // Script:  dev2
    // Feature: blwf
    //
    // Lookup:
    //
    //     glyph 20 -> glyph 30
    //
    // The resolver probes:
    //
    //     Halant + candidate
    //
    // so candidate glyph 20 is classified as below-base, while any other
    // candidate glyph is left unchanged.
    // ========================================================================

    static inline std::vector<uint8_t> makeIndicBaseTestGsub()
    {
        std::vector<uint8_t> data;


        // ------------------------------------------------------------
        // GSUB header.
        //
        // Header       0
        // ScriptList  10
        // FeatureList 30
        // LookupList  44
        // ------------------------------------------------------------

        indicBaseTestPushU16(data, 1);
        indicBaseTestPushU16(data, 0);
        indicBaseTestPushU16(data, 10);
        indicBaseTestPushU16(data, 30);
        indicBaseTestPushU16(data, 44);


        // ------------------------------------------------------------
        // ScriptList @ 10
        //
        // ScriptRecord:
        //     dev2 -> +8
        //
        // Script:
        //     DefaultLangSys -> +4
        // ------------------------------------------------------------

        indicBaseTestPushU16(data, 1);

        indicBaseTestPushU32(data, OTAG("dev2"));
        indicBaseTestPushU16(data, 8);

        // Script table.
        indicBaseTestPushU16(data, 4);
        indicBaseTestPushU16(data, 0);

        // DefaultLangSys.
        indicBaseTestPushU16(data, 0);
        indicBaseTestPushU16(data, 0xFFFFu);
        indicBaseTestPushU16(data, 1);
        indicBaseTestPushU16(data, 0);


        // ------------------------------------------------------------
        // FeatureList @ 30
        //
        // FeatureRecord:
        //     blwf -> +8
        //
        // Feature:
        //     lookup 0
        // ------------------------------------------------------------

        indicBaseTestPushU16(data, 1);

        indicBaseTestPushU32(data, OTAG("blwf"));
        indicBaseTestPushU16(data, 8);

        indicBaseTestPushU16(data, 0);
        indicBaseTestPushU16(data, 1);
        indicBaseTestPushU16(data, 0);


        // ------------------------------------------------------------
        // LookupList @ 44
        //
        // lookup 0 -> +4
        // ------------------------------------------------------------

        indicBaseTestPushU16(data, 1);
        indicBaseTestPushU16(data, 4);


        // ------------------------------------------------------------
        // Lookup 0
        //
        // SingleSubst lookup, one subtable.
        // ------------------------------------------------------------

        indicBaseTestPushU16(data, 1);
        indicBaseTestPushU16(data, 0);
        indicBaseTestPushU16(data, 1);
        indicBaseTestPushU16(data, 8);


        // ------------------------------------------------------------
        // SingleSubst Format 2
        //
        // glyph 20 -> 30
        //
        // Coverage follows substitute array.
        // ------------------------------------------------------------

        indicBaseTestPushU16(data, 2);
        indicBaseTestPushU16(data, 8);
        indicBaseTestPushU16(data, 1);
        indicBaseTestPushU16(data, 30);

        // Coverage Format 1.
        indicBaseTestPushU16(data, 1);
        indicBaseTestPushU16(data, 1);
        indicBaseTestPushU16(data, 20);

        return data;
    }


    static inline void makeIndicBaseTestRecognition(
        ScriptRecognitionResult& recognition,
        ScriptRecognitionUnit& unit)
    {
        static constexpr ScriptItemKindId kConsonant = 2;
        static constexpr ScriptItemKindId kHalant = 4;

        static constexpr ScriptRoleId kConsonantSequence = 1;
        static constexpr ScriptRoleId kBaseCandidate = 2;

        recognition.clear();

        recognition.kinds =
        {
            kConsonant,
            kHalant,
            kConsonant
        };

        recognition.roles.push_back({
            kConsonantSequence,
            0,
            { 0, 2 }
            });

        recognition.roles.push_back({
            kBaseCandidate,
            0,
            { 2, 1 }
            });

        unit = {};
        unit.span = { 0, 3 };
        unit.roleOffset = 0;
        unit.roleCount = 2;
    }


    static inline ScriptShapingIRResolveIndicBase makeIndicBaseTestResolver()
    {
        static constexpr ScriptItemKindId kRa = 1;
        static constexpr ScriptItemKindId kConsonant = 2;
        static constexpr ScriptItemKindId kNukta = 3;
        static constexpr ScriptItemKindId kHalant = 4;
        static constexpr ScriptItemKindId kZWJ = 5;
        static constexpr ScriptItemKindId kZWNJ = 6;

        static constexpr ScriptRoleId kConsonantSequence = 1;
        static constexpr ScriptRoleId kBaseCandidate = 2;

        static constexpr ScriptShapingSelectionId kBaseSelection = 1;

        ScriptShapingIRResolveIndicBase resolver{};

        resolver.consonantSequence =
            scriptShapingRoleSelection(kConsonantSequence);

        resolver.baseCandidate =
            scriptShapingRoleSelection(kBaseCandidate);

        resolver.outputBaseSelection =
            kBaseSelection;

        resolver.raKind = kRa;
        resolver.consonantKind = kConsonant;
        resolver.nuktaKind = kNukta;
        resolver.halantKind = kHalant;
        resolver.zwjKind = kZWJ;
        resolver.zwnjKind = kZWNJ;
        resolver.model = ScriptShapingIndicModel::New;

        return resolver;
    }


    static inline void makeIndicBaseTestBuffer(
        const FontRunView& run,
        uint16_t baseGlyph,
        OpenTypeShapingBuffer& buffer)
    {
        buffer.reset(run);

        OpenTypeShapingGlyph c1{};
        c1.glyphId = 10;
        c1.scalarOffset = 0;
        c1.scalarCount = 1;

        OpenTypeShapingGlyph halant{};
        halant.glyphId = 15;
        halant.scalarOffset = 1;
        halant.scalarCount = 1;

        OpenTypeShapingGlyph c2{};
        c2.glyphId = baseGlyph;
        c2.scalarOffset = 2;
        c2.scalarCount = 1;

        buffer.pushBack(c1);
        buffer.pushBack(halant);
        buffer.pushBack(c2);
    }


    static bool testScriptShapingIndicBaseGsub()
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "Script shaping Indic base GSUB: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        const std::vector<uint8_t> gsubBytes =
            makeIndicBaseTestGsub();

        const ByteSpan gsub(
            gsubBytes.data(),
            gsubBytes.size());

        ScriptRecognitionResult recognition;
        ScriptRecognitionUnit unit;

        makeIndicBaseTestRecognition(
            recognition,
            unit);

        const ScriptShapingIRResolveIndicBase resolver =
            makeIndicBaseTestResolver();

        FontRunView run{};


        // ============================================================
        // Case 1:
        //
        // C1 Halant C2
        //
        // C2 glyph = 21.
        //
        // blwf only covers glyph 20, so C2 does not change.
        //
        // Expected:
        //
        //     Base = C2
        // ============================================================

        {
            OpenTypeShapingBuffer buffer;

            makeIndicBaseTestBuffer(
                run,
                21,
                buffer);

            ScriptShapingSelectionState selectionState;

            if (!selectionState.reset(1))
                return fail("unable to initialize unmatched selection state");

            if (!applyScriptShapingIRResolveIndicBase(
                gsub,
                OTAG("dev2"),
                0,
                resolver,
                true,
                true,
                {},
                recognition,
                unit,
                selectionState,
                buffer))
            {
                return fail("unmatched blwf resolver execution failed");
            }

            const ScriptShapingDerivedSelection* base =
                selectionState.selection(
                    resolver.outputBaseSelection);

            if (!base)
                return fail("unmatched blwf Base selection missing");

            if (base->sourceSpans.size() != 1)
                return fail("unmatched blwf Base span count is incorrect");

            if (base->sourceSpans[0].first != 2 ||
                base->sourceSpans[0].count != 1)
            {
                return fail("unmatched blwf should keep C2 as Base");
            }
        }


        // ============================================================
        // Case 2:
        //
        // C1 Halant C2
        //
        // C2 glyph = 20.
        //
        // blwf changes:
        //
        //     20 -> 30
        //
        // so C2 is classified as below-base and skipped.
        //
        // Expected:
        //
        //     Base = C1
        // ============================================================

        {
            OpenTypeShapingBuffer buffer;

            makeIndicBaseTestBuffer(
                run,
                20,
                buffer);

            ScriptShapingSelectionState selectionState;

            if (!selectionState.reset(1))
                return fail("unable to initialize matched selection state");

            if (!applyScriptShapingIRResolveIndicBase(
                gsub,
                OTAG("dev2"),
                0,
                resolver,
                true,
                true,
                {},
                recognition,
                unit,
                selectionState,
                buffer))
            {
                return fail("matched blwf resolver execution failed");
            }

            const ScriptShapingDerivedSelection* base =
                selectionState.selection(
                    resolver.outputBaseSelection);

            if (!base)
                return fail("matched blwf Base selection missing");

            if (base->sourceSpans.size() != 1)
                return fail("matched blwf Base span count is incorrect");

            if (base->sourceSpans[0].first != 0 ||
                base->sourceSpans[0].count != 1)
            {
                return fail("matched blwf should move Base to C1");
            }
        }


        std::printf(
            "Script shaping Indic base GSUB: PASS\n"
            "  Synthetic GSUB:            PASS\n"
            "  No blwf match -> C2 Base:  PASS\n"
            "  blwf match -> C1 Base:     PASS\n"
            "  Feature probe path:        PASS\n");

        return true;
    }

} // namespace waavs