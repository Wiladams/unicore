// test_shaping_devanagari.h
#pragma once

#include "test_core.h"

#include <cstdio>

#include "shaping_devanagari.h"
#include "script_recognition_dsl.h"
#include "script_shaping_ir_builder.h"

namespace waavs
{
    static bool testDevanagariShapingBuilder()
    {
        auto fail =
            [](const char* message)
            {
                std::printf(
                    "Devanagari shaping builder: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };

        ScriptRecognitionDSL grammar;
        DevanagariItemKinds kinds;

        if (!defineDevanagariItemKinds(grammar, kinds))
            return fail("unable to define item kinds");

        DevanagariRecognition recognition;

        if (!defineDevanagariRecognition(grammar, kinds, recognition))
            return fail("unable to define recognition");

        if (!kinds.valid() || !recognition.valid())
            return fail("recognition vocabulary is invalid");

        ScriptShapingIRBuilder builder;
        DevanagariShapingSelections selections;

        if (!appendDevanagariShaping(kinds, recognition, builder, selections))
            return fail("unable to append Devanagari shaping");

        if (!selections.valid())
            return fail("shaping selections are invalid");

        ScriptShapingIR ir;

        if (!builder.finalize(ir))
            return fail("unable to finalize shaping IR");

        if (ir.instructions.size() != 22)
            return fail("expected exactly twenty-two shaping instructions");

        static constexpr ScriptShapingIROp kExpectedOps[] =
        {
            ScriptShapingIROp::ResolveIndicBase,
            ScriptShapingIROp::ResolveIndicPreBaseInitialAnchor,
            ScriptShapingIROp::MoveSelection,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::ResolveIndicHalfCandidates,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::ResolveIndicPreBaseAnchor,
            ScriptShapingIROp::MoveSelection,
            ScriptShapingIROp::MoveSelection,
            ScriptShapingIROp::ResolveIndicRephAnchor,
            ScriptShapingIROp::MoveSelection,
            ScriptShapingIROp::GsubFeatureStage,
            ScriptShapingIROp::GposFeatureStage
        };

        for (size_t i = 0; i < sizeof(kExpectedOps) / sizeof(kExpectedOps[0]); ++i)
        {
            if (ir.instructions[i].op != kExpectedOps[i])
                return fail("instruction order mismatch");
        }

        struct ExpectedFeature
        {
            size_t instructionIndex;
            uint32_t tag;
            ScriptShapingSelectionKind inputKind;
        };

        static constexpr ExpectedFeature kBasicFeatures[] =
        {
            { 3, OTAG("locl"), ScriptShapingSelectionKind::Unit },
            { 4, OTAG("nukt"), ScriptShapingSelectionKind::Unit },
            { 5, OTAG("akhn"), ScriptShapingSelectionKind::Unit },
            { 6, OTAG("rphf"), ScriptShapingSelectionKind::Role },
            { 7, OTAG("rkrf"), ScriptShapingSelectionKind::Unit },
            { 8, OTAG("pref"), ScriptShapingSelectionKind::Unit },
            { 9, OTAG("blwf"), ScriptShapingSelectionKind::Unit },
            { 11, OTAG("half"), ScriptShapingSelectionKind::Derived },
            { 12, OTAG("pstf"), ScriptShapingSelectionKind::Unit },
            { 13, OTAG("vatu"), ScriptShapingSelectionKind::Unit },
            { 14, OTAG("cjct"), ScriptShapingSelectionKind::Unit }
        };

        for (const ExpectedFeature& expected : kBasicFeatures)
        {
            const ScriptShapingIRInstruction& instruction = ir.instructions[expected.instructionIndex];
            const ScriptShapingIRFeatureStage* stage = ir.featureStage(instruction.payloadIndex);

            if (!stage || stage->featureCount != 1)
                return fail("basic feature stage is malformed");

            const uint32_t* tags = ir.featureTagData(*stage);

            if (!tags || tags[0] != expected.tag)
                return fail("basic feature order/tag mismatch");

            if (stage->inputSelection.kind != expected.inputKind)
                return fail("basic feature has wrong input selection kind");
        }

        const ScriptShapingIRFeatureStage* locl = ir.featureStage(ir.instructions[3].payloadIndex);

        if (!locl || locl->includeRequiredFeature != 1)
            return fail("locl must include required feature");

        const ScriptShapingIRFeatureStage* rphf = ir.featureStage(ir.instructions[6].payloadIndex);

        if (!rphf || rphf->inputSelection.id != recognition.rephCandidate.id ||
            rphf->outputChangedSelection != selections.reph)
        {
            return fail("rphf selection wiring is incorrect");
        }

        const ScriptShapingIRFeatureStage* pref = ir.featureStage(ir.instructions[8].payloadIndex);
        const ScriptShapingIRFeatureStage* half = ir.featureStage(ir.instructions[11].payloadIndex);
        const ScriptShapingIRFeatureStage* pstf = ir.featureStage(ir.instructions[12].payloadIndex);

        if (!pref || pref->outputChangedSelection != selections.prefForms)
            return fail("pref changed-output selection is incorrect");

        if (!half || half->inputSelection.id != selections.halfCandidates ||
            half->outputChangedSelection != selections.halfForms)
        {
            return fail("half selection wiring is incorrect");
        }

        if (!pstf || pstf->outputChangedSelection != selections.postBaseForms)
            return fail("pstf changed-output selection is incorrect");

        const ScriptShapingIRFeatureStage* presentation = ir.featureStage(ir.instructions[20].payloadIndex);

        if (!presentation || presentation->featureCount != 6 ||
            presentation->inputSelection.kind != ScriptShapingSelectionKind::Unit)
        {
            return fail("presentation stage is incorrect");
        }

        static constexpr uint32_t kPresentation[] =
        {
            OTAG("pres"), OTAG("abvs"), OTAG("blws"),
            OTAG("psts"), OTAG("haln"), OTAG("calt")
        };

        const uint32_t* presentationTags = ir.featureTagData(*presentation);

        for (size_t i = 0; i < 6; ++i)
        {
            if (!presentationTags || presentationTags[i] != kPresentation[i])
                return fail("presentation feature tags are incorrect");
        }

        const ScriptShapingIRFeatureStage* positioning = ir.featureStage(ir.instructions[21].payloadIndex);

        if (!positioning || positioning->featureCount != 4)
            return fail("positioning stage is incorrect");

        static constexpr uint32_t kPositioning[] =
        {
            OTAG("kern"), OTAG("dist"), OTAG("abvm"), OTAG("blwm")
        };

        const uint32_t* positioningTags = ir.featureTagData(*positioning);

        for (size_t i = 0; i < 4; ++i)
        {
            if (!positioningTags || positioningTags[i] != kPositioning[i])
                return fail("positioning feature tags are incorrect");
        }

        const ScriptShapingIRResolveIndicBase* base = ir.indicBaseResolver(ir.instructions[0].payloadIndex);

        if (!base || base->zwnjKind != kinds.zwnj.id || base->model != ScriptShapingIndicModel::Auto)
            return fail("Indic Base resolver vocabulary/model is incorrect");

        std::printf(
            "Devanagari shaping builder: PASS\n"
            "  Recognition vocabulary:   PASS\n"
            "  Unit-scoped GSUB:          PASS\n"
            "  Indic Base first:          PASS\n"
            "  Initial matra reorder:     PASS\n"
            "  Basic feature order:       PASS\n"
            "  ZWNJ half filtering:       PASS\n"
            "  pref capture:              PASS\n"
            "  pstf capture:              PASS\n"
            "  Final matra reorder:       PASS\n"
            "  Final Reph reorder:        PASS\n"
            "  Presentation features:     PASS\n"
            "  Positioning features:      PASS\n");

        return true;
    }

} // namespace waavs
