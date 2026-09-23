// shaping_devanagari.h
#pragma once

#include "opentype_tags.h"
#include "recognition_devanagari.h"
#include "script_shaping_ir_builder.h"

namespace waavs
{
    struct DevanagariShapingSelections
    {
        ScriptShapingSelectionId base{ kScriptShapingSelectionInvalid };
        ScriptShapingSelectionId reph{ kScriptShapingSelectionInvalid };
        ScriptShapingSelectionId halfCandidates{ kScriptShapingSelectionInvalid };
        ScriptShapingSelectionId halfForms{ kScriptShapingSelectionInvalid };
        ScriptShapingSelectionId prefForms{ kScriptShapingSelectionInvalid };
        ScriptShapingSelectionId postBaseForms{ kScriptShapingSelectionInvalid };
        ScriptShapingSelectionId preBaseInitialAnchor{ kScriptShapingSelectionInvalid };
        ScriptShapingSelectionId preBaseFinalAnchor{ kScriptShapingSelectionInvalid };
        ScriptShapingSelectionId rephAnchor{ kScriptShapingSelectionInvalid };

        [[nodiscard]]
        bool valid() const noexcept
        {
            return base != kScriptShapingSelectionInvalid &&
                reph != kScriptShapingSelectionInvalid &&
                halfCandidates != kScriptShapingSelectionInvalid &&
                halfForms != kScriptShapingSelectionInvalid &&
                prefForms != kScriptShapingSelectionInvalid &&
                postBaseForms != kScriptShapingSelectionInvalid &&
                preBaseInitialAnchor != kScriptShapingSelectionInvalid &&
                preBaseFinalAnchor != kScriptShapingSelectionInvalid &&
                rephAnchor != kScriptShapingSelectionInvalid;
        }
    };


    [[nodiscard]]
    static inline bool appendDevanagariShaping(
        const DevanagariItemKinds& kinds,
        const DevanagariRecognition& recognition,
        ScriptShapingIRBuilder& builder,
        DevanagariShapingSelections& selections)
    {
        if (!kinds.valid() || !recognition.valid())
            return false;

        DevanagariShapingSelections working{};

        working.base = builder.addDerivedSelection();
        working.reph = builder.addDerivedSelection();
        working.halfCandidates = builder.addDerivedSelection();
        working.halfForms = builder.addDerivedSelection();
        working.prefForms = builder.addDerivedSelection();
        working.postBaseForms = builder.addDerivedSelection();
        working.preBaseInitialAnchor = builder.addDerivedSelection();
        working.preBaseFinalAnchor = builder.addDerivedSelection();
        working.rephAnchor = builder.addDerivedSelection();

        if (!working.valid())
            return false;


        // ------------------------------------------------------------
        // Initial Indic analysis and reordering.
        // ------------------------------------------------------------

        if (!builder.addResolveIndicBase(
            scriptShapingRoleSelection(recognition.consonantSequence.id),
            scriptShapingRoleSelection(recognition.baseCandidate.id),
            scriptShapingRoleSelection(recognition.rephCandidate.id),
            working.base,
            kinds.ra.id, kinds.consonant.id, kinds.nukta.id,
            kinds.halant.id, kinds.zwj.id, kinds.zwnj.id,
            ScriptShapingIndicModel::Auto))
        {
            return false;
        }

        if (!builder.addResolveIndicPreBaseInitialAnchor(
            scriptShapingRoleSelection(recognition.consonantSequence.id),
            scriptShapingRoleSelection(recognition.baseCandidate.id),
            working.preBaseInitialAnchor))
        {
            return false;
        }

        if (!builder.addMoveSelection(
            scriptShapingRoleSelection(recognition.preBaseMatras.id),
            scriptShapingDerivedSelection(working.preBaseInitialAnchor),
            ScriptShapingIRMovePlacement::Before))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Localized and basic shaping forms.
        // ------------------------------------------------------------

        static constexpr uint32_t kLocl[] = { OTAG("locl") };
        static constexpr uint32_t kNukt[] = { OTAG("nukt") };
        static constexpr uint32_t kAkhn[] = { OTAG("akhn") };
        static constexpr uint32_t kRphf[] = { OTAG("rphf") };
        static constexpr uint32_t kRkrf[] = { OTAG("rkrf") };
        static constexpr uint32_t kPref[] = { OTAG("pref") };
        static constexpr uint32_t kBlwf[] = { OTAG("blwf") };
        static constexpr uint32_t kHalf[] = { OTAG("half") };
        static constexpr uint32_t kPstf[] = { OTAG("pstf") };
        static constexpr uint32_t kVatu[] = { OTAG("vatu") };
        static constexpr uint32_t kCjct[] = { OTAG("cjct") };

        const ScriptShapingSelectionRef unit = scriptShapingUnitSelection();

        if (!builder.addGsubFeatureStage(kLocl, unit, kScriptShapingSelectionInvalid, true)) return false;
        if (!builder.addGsubFeatureStage(kNukt, unit)) return false;
        if (!builder.addGsubFeatureStage(kAkhn, unit)) return false;

        if (!builder.addGsubFeatureStage(
            kRphf,
            scriptShapingRoleSelection(recognition.rephCandidate.id),
            working.reph))
        {
            return false;
        }

        if (!builder.addGsubFeatureStage(kRkrf, unit)) return false;
        if (!builder.addGsubFeatureStage(kPref, unit, working.prefForms)) return false;
        if (!builder.addGsubFeatureStage(kBlwf, unit)) return false;

        if (!builder.addResolveIndicHalfCandidates(
            scriptShapingRoleSelection(recognition.consonantSequence.id),
            working.halfCandidates,
            kinds.ra.id, kinds.consonant.id, kinds.nukta.id,
            kinds.halant.id, kinds.zwj.id, kinds.zwnj.id))
        {
            return false;
        }

        if (!builder.addGsubFeatureStage(
            kHalf,
            scriptShapingDerivedSelection(working.halfCandidates),
            working.halfForms))
        {
            return false;
        }

        if (!builder.addGsubFeatureStage(kPstf, unit, working.postBaseForms)) return false;
        if (!builder.addGsubFeatureStage(kVatu, unit)) return false;
        if (!builder.addGsubFeatureStage(kCjct, unit)) return false;


        // ------------------------------------------------------------
        // Final Indic reordering.
        // ------------------------------------------------------------

        if (!builder.addResolveIndicPreBaseAnchor(
            scriptShapingDerivedSelection(working.base),
            working.preBaseFinalAnchor,
            kinds.halant.id, kinds.zwj.id, kinds.zwnj.id))
        {
            return false;
        }

        if (!builder.addMoveSelection(
            scriptShapingRoleSelection(recognition.preBaseMatras.id),
            scriptShapingDerivedSelection(working.preBaseFinalAnchor),
            ScriptShapingIRMovePlacement::Before))
        {
            return false;
        }

        if (!builder.addMoveSelection(
            scriptShapingDerivedSelection(working.prefForms),
            scriptShapingDerivedSelection(working.preBaseFinalAnchor),
            ScriptShapingIRMovePlacement::Before))
        {
            return false;
        }

        if (!builder.addResolveIndicRephAnchor(
            scriptShapingDerivedSelection(working.reph),
            scriptShapingDerivedSelection(working.base),
            scriptShapingDerivedSelection(working.postBaseForms),
            working.rephAnchor,
            kinds.ra.id, kinds.consonant.id, kinds.nukta.id,
            kinds.halant.id, kinds.zwj.id, kinds.zwnj.id))
        {
            return false;
        }

        if (!builder.addMoveSelection(
            scriptShapingDerivedSelection(working.reph),
            scriptShapingDerivedSelection(working.rephAnchor),
            ScriptShapingIRMovePlacement::After))
        {
            return false;
        }


        // ------------------------------------------------------------
        // Presentation forms are selected together over the current unit.
        // ------------------------------------------------------------

        static constexpr uint32_t kPresentation[] =
        {
            OTAG("pres"), OTAG("abvs"), OTAG("blws"),
            OTAG("psts"), OTAG("haln"), OTAG("calt")
        };

        if (!builder.addGsubFeatureStage(kPresentation, unit))
            return false;


        // ------------------------------------------------------------
        // GPOS features share the existing single attachment graph.
        // ------------------------------------------------------------

        static constexpr uint32_t kPositioning[] =
        {
            OTAG("kern"), OTAG("dist"), OTAG("abvm"), OTAG("blwm")
        };

        if (!builder.addGposFeatureStage(kPositioning, false))
            return false;

        selections = working;
        return true;
    }

} // namespace waavs
