// test_script_recognition_dsl_models.h
#pragma once

#include "test_core.h"

#include "script_recognition_dsl.h"

namespace waavs
{
    static inline bool descriptionHasExprKind(const ScriptRecognitionDescription& description, ScriptRecognitionExprKind kind)
    {
        for (const ScriptRecognitionExpr& expr : description.expressions)
        {
            if (expr.kind == kind)
                return true;
        }

        return false;
    }


    static inline bool descriptionHasRoleCapture(const ScriptRecognitionDescription& description, ScriptRoleId roleId)
    {
        for (const ScriptRecognitionExpr& expr : description.expressions)
        {
            if (expr.kind == ScriptRecognitionExprKind::Capture && expr.roleId == roleId)
                return true;
        }

        return false;
    }


    // ------------------------------------------------------------------------
    // Devanagari
    //
    // Conceptual shape:
    //
    //     (RephCandidate)?
    //     (Consonant Nukta? Halant)*
    //     Base
    //     PreBaseMatra?
    //
    // This intentionally exercises nested sequences, optional expressions,
    // repetition, and role captures.
    // ------------------------------------------------------------------------

    static inline bool testRecognitionModelDevanagari()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef ra = dsl.kind("Ra");
        const ScriptItemKindRef halant = dsl.kind("Halant");
        const ScriptItemKindRef consonant = dsl.kind("Consonant");
        const ScriptItemKindRef nukta = dsl.kind("Nukta");
        const ScriptItemKindRef matraPre = dsl.kind("MatraPre");

        const ScriptRoleRef rephCandidate = dsl.role("RephCandidate");
        const ScriptRoleRef base = dsl.role("Base");
        const ScriptRoleRef preBaseMatra = dsl.role("PreBaseMatra");

        if (!ra || !halant || !consonant || !nukta || !matraPre)
            return false;

        if (!rephCandidate || !base || !preBaseMatra)
            return false;

        const ScriptExprRef Ra = dsl.match(ra);
        const ScriptExprRef H = dsl.match(halant);
        const ScriptExprRef C = dsl.match(consonant);
        const ScriptExprRef N = dsl.match(nukta);
        const ScriptExprRef MPre = dsl.match(matraPre);

        const ScriptExprRef reph =
            dsl.capture(
                rephCandidate,
                dsl.seq({
                    Ra,
                    H
                    }));

        const ScriptExprRef prefixConsonant =
            dsl.seq({
                C,
                dsl.opt(N),
                H
                });

        const ScriptExprRef syllable =
            dsl.seq({
                dsl.opt(reph),
                dsl.zeroOrMore(prefixConsonant),
                dsl.capture(base, C),
                dsl.opt(dsl.capture(preBaseMatra, MPre))
                });

        if (!syllable)
            return false;

        if (!dsl.unit("ConsonantSyllable", syllable))
            return false;

        const ScriptRecognitionDescription& description = dsl.description();

        if (description.kinds.size() != 5)
            return false;

        if (description.roles.size() != 3)
            return false;

        if (description.units.size() != 1)
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::Sequence))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::Optional))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::ZeroOrMore))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::Capture))
            return false;

        if (!descriptionHasRoleCapture(description, rephCandidate.id))
            return false;

        if (!descriptionHasRoleCapture(description, base.id))
            return false;

        if (!descriptionHasRoleCapture(description, preBaseMatra.id))
            return false;

        return true;
    }


    // ------------------------------------------------------------------------
    // Arabic
    //
    // Conceptual shape:
    //
    //     JoiningItem+
    //
    // Recognition here is intentionally very different from Indic. There are
    // no structural roles; the important information is the sequence of
    // joining kinds from which later shaping state can be derived.
    // ------------------------------------------------------------------------

    static inline bool testRecognitionModelArabic()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef dual = dsl.kind("DualJoining");
        const ScriptItemKindRef right = dsl.kind("RightJoining");
        const ScriptItemKindRef transparent = dsl.kind("Transparent");
        const ScriptItemKindRef joinCausing = dsl.kind("JoinCausing");

        if (!dual || !right || !transparent || !joinCausing)
            return false;

        const ScriptExprRef joiningItem =
            dsl.choice({
                dsl.match(dual),
                dsl.match(right),
                dsl.match(transparent),
                dsl.match(joinCausing)
                });

        const ScriptExprRef joiningRun =
            dsl.oneOrMore(joiningItem);

        if (!joiningRun)
            return false;

        if (!dsl.unit("JoiningRun", joiningRun))
            return false;

        const ScriptRecognitionDescription& description = dsl.description();

        if (description.kinds.size() != 4)
            return false;

        if (!description.roles.empty())
            return false;

        if (description.units.size() != 1)
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::Choice))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::OneOrMore))
            return false;

        return true;
    }


    // ------------------------------------------------------------------------
    // Myanmar
    //
    // Conceptual shape:
    //
    //     Kinzi?
    //     Base
    //     PreBaseVowel*
    //
    // Kinzi and the pre-base vowel group demonstrate that a role may bind a
    // multi-item span rather than only one scalar.
    // ------------------------------------------------------------------------

    static inline bool testRecognitionModelMyanmar()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef kinziConsonant = dsl.kind("KinziConsonant");
        const ScriptItemKindRef asat = dsl.kind("Asat");
        const ScriptItemKindRef virama = dsl.kind("Virama");
        const ScriptItemKindRef consonant = dsl.kind("Consonant");
        const ScriptItemKindRef vowelPre = dsl.kind("VowelPre");

        const ScriptRoleRef kinzi = dsl.role("Kinzi");
        const ScriptRoleRef base = dsl.role("Base");
        const ScriptRoleRef preBaseVowels = dsl.role("PreBaseVowels");

        if (!kinziConsonant || !asat || !virama || !consonant || !vowelPre)
            return false;

        if (!kinzi || !base || !preBaseVowels)
            return false;

        const ScriptExprRef K = dsl.match(kinziConsonant);
        const ScriptExprRef A = dsl.match(asat);
        const ScriptExprRef V = dsl.match(virama);
        const ScriptExprRef C = dsl.match(consonant);
        const ScriptExprRef VP = dsl.match(vowelPre);

        const ScriptExprRef kinziSequence =
            dsl.capture(
                kinzi,
                dsl.seq({
                    K,
                    A,
                    V
                    }));

        const ScriptExprRef syllable =
            dsl.seq({
                dsl.opt(kinziSequence),
                dsl.capture(base, C),
                dsl.opt(
                    dsl.capture(
                        preBaseVowels,
                        dsl.oneOrMore(VP)))
                });

        if (!syllable)
            return false;

        if (!dsl.unit("MyanmarSyllable", syllable))
            return false;

        const ScriptRecognitionDescription& description = dsl.description();

        if (description.kinds.size() != 5)
            return false;

        if (description.roles.size() != 3)
            return false;

        if (description.units.size() != 1)
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::OneOrMore))
            return false;

        if (!descriptionHasRoleCapture(description, kinzi.id))
            return false;

        if (!descriptionHasRoleCapture(description, base.id))
            return false;

        if (!descriptionHasRoleCapture(description, preBaseVowels.id))
            return false;

        return true;
    }


    // ------------------------------------------------------------------------
    // Hangul
    //
    // Conceptual shape:
    //
    //     L V T?
    //
    // This deliberately uses no roles or repetition beyond Optional. It is a
    // useful check that the model does not force complex-script machinery onto
    // a structurally simple recognizer.
    // ------------------------------------------------------------------------

    static inline bool testRecognitionModelHangul()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef leading = dsl.kind("LeadingJamo");
        const ScriptItemKindRef vowel = dsl.kind("VowelJamo");
        const ScriptItemKindRef trailing = dsl.kind("TrailingJamo");

        if (!leading || !vowel || !trailing)
            return false;

        const ScriptExprRef composition =
            dsl.seq({
                dsl.match(leading),
                dsl.match(vowel),
                dsl.opt(dsl.match(trailing))
                });

        if (!composition)
            return false;

        if (!dsl.unit("JamoSequence", composition))
            return false;

        const ScriptRecognitionDescription& description = dsl.description();

        if (description.kinds.size() != 3)
            return false;

        if (!description.roles.empty())
            return false;

        if (description.units.size() != 1)
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::Sequence))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::Optional))
            return false;

        return true;
    }


    // ------------------------------------------------------------------------
    // Emoji
    //
    // Conceptual shape:
    //
    //     ExtendedPictographic Extend* ZWJ ExtendedPictographic
    //
    // This gives us a recognizer that is sequence-oriented but neither
    // syllabic nor joining-oriented.
    // ------------------------------------------------------------------------

    static inline bool testRecognitionModelEmoji()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef pictographic = dsl.kind("ExtendedPictographic");
        const ScriptItemKindRef extend = dsl.kind("Extend");
        const ScriptItemKindRef zwj = dsl.kind("ZWJ");

        if (!pictographic || !extend || !zwj)
            return false;

        const ScriptExprRef EP = dsl.match(pictographic);
        const ScriptExprRef Extend = dsl.match(extend);
        const ScriptExprRef ZWJ = dsl.match(zwj);

        const ScriptExprRef sequence =
            dsl.seq({
                EP,
                dsl.zeroOrMore(Extend),
                ZWJ,
                EP
                });

        if (!sequence)
            return false;

        if (!dsl.unit("EmojiZWJSequence", sequence))
            return false;

        const ScriptRecognitionDescription& description = dsl.description();

        if (description.kinds.size() != 3)
            return false;

        if (!description.roles.empty())
            return false;

        if (description.units.size() != 1)
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::ZeroOrMore))
            return false;

        return true;
    }


    // ------------------------------------------------------------------------
// Khmer
//
// Conceptual shape:
//
//     Base
//     (COENG + RO | COENG + Consonant)*
//     PreBaseVowel?
//     Mark*
//
// This stresses:
//
//     - repeated compound expressions
//     - Choice inside repetition
//     - a role bound to a multi-item COENG + RO span
//
// This is an architectural model, not the complete Khmer grammar.
// ------------------------------------------------------------------------

    static inline bool testRecognitionModelKhmer()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef consonant = dsl.kind("Consonant");
        const ScriptItemKindRef coeng = dsl.kind("Coeng");
        const ScriptItemKindRef ro = dsl.kind("Ro");
        const ScriptItemKindRef vowelPre = dsl.kind("VowelPre");
        const ScriptItemKindRef mark = dsl.kind("Mark");

        const ScriptRoleRef base = dsl.role("Base");
        const ScriptRoleRef preBaseSubscript = dsl.role("PreBaseSubscript");
        const ScriptRoleRef preBaseVowel = dsl.role("PreBaseVowel");

        if (!consonant || !coeng || !ro || !vowelPre || !mark)
            return false;

        if (!base || !preBaseSubscript || !preBaseVowel)
            return false;

        const ScriptExprRef C = dsl.match(consonant);
        const ScriptExprRef Coeng = dsl.match(coeng);
        const ScriptExprRef Ro = dsl.match(ro);
        const ScriptExprRef VPre = dsl.match(vowelPre);
        const ScriptExprRef Mark = dsl.match(mark);

        const ScriptExprRef coengRo =
            dsl.capture(
                preBaseSubscript,
                dsl.seq({
                    Coeng,
                    Ro
                    }));

        const ScriptExprRef coengConsonant =
            dsl.seq({
                Coeng,
                C
                });

        const ScriptExprRef subscript =
            dsl.choice({
                coengRo,
                coengConsonant
                });

        const ScriptExprRef syllable =
            dsl.seq({
                dsl.capture(base, C),
                dsl.zeroOrMore(subscript),
                dsl.opt(dsl.capture(preBaseVowel, VPre)),
                dsl.zeroOrMore(Mark)
                });

        if (!syllable)
            return false;

        if (!dsl.unit("KhmerSyllable", syllable))
            return false;

        const ScriptRecognitionDescription& description = dsl.description();

        if (description.kinds.size() != 5)
            return false;

        if (description.roles.size() != 3)
            return false;

        if (description.units.size() != 1)
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::Choice))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::ZeroOrMore))
            return false;

        if (!descriptionHasRoleCapture(description, base.id))
            return false;

        if (!descriptionHasRoleCapture(description, preBaseSubscript.id))
            return false;

        if (!descriptionHasRoleCapture(description, preBaseVowel.id))
            return false;

        return true;
    }


    // ------------------------------------------------------------------------
    // Tibetan
    //
    // Conceptual shape:
    //
    //     Base
    //     Subjoined*
    //     Vowel?
    //     Mark*
    //
    // Tibetan stacking gives us another useful case where one role can bind
    // an arbitrary-length structural span.
    //
    // This is an architectural model, not the complete Tibetan grammar.
    // ------------------------------------------------------------------------

    static inline bool testRecognitionModelTibetan()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef baseLetter = dsl.kind("BaseLetter");
        const ScriptItemKindRef subjoined = dsl.kind("Subjoined");
        const ScriptItemKindRef vowel = dsl.kind("Vowel");
        const ScriptItemKindRef mark = dsl.kind("Mark");

        const ScriptRoleRef base = dsl.role("Base");
        const ScriptRoleRef stack = dsl.role("SubjoinedStack");

        if (!baseLetter || !subjoined || !vowel || !mark)
            return false;

        if (!base || !stack)
            return false;

        const ScriptExprRef B = dsl.match(baseLetter);
        const ScriptExprRef S = dsl.match(subjoined);
        const ScriptExprRef V = dsl.match(vowel);
        const ScriptExprRef M = dsl.match(mark);

        const ScriptExprRef syllable =
            dsl.seq({
                dsl.capture(base, B),
                dsl.opt(
                    dsl.capture(
                        stack,
                        dsl.oneOrMore(S))),
                dsl.opt(V),
                dsl.zeroOrMore(M)
                });

        if (!syllable)
            return false;

        if (!dsl.unit("TibetanStack", syllable))
            return false;

        const ScriptRecognitionDescription& description = dsl.description();

        if (description.kinds.size() != 4)
            return false;

        if (description.roles.size() != 2)
            return false;

        if (description.units.size() != 1)
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::OneOrMore))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::ZeroOrMore))
            return false;

        if (!descriptionHasRoleCapture(description, base.id))
            return false;

        if (!descriptionHasRoleCapture(description, stack.id))
            return false;

        return true;
    }


    // ------------------------------------------------------------------------
    // USE-style syllabic model
    //
    // Conceptual shape:
    //
    //     Repha?
    //     Base
    //     (Halant Consonant)*
    //     Medial*
    //     Vowel*
    //     Mark*
    //
    // USE covers many scripts with a common syllabic shaping framework.
    // The purpose here is to make sure that the generic vocabulary naturally
    // expresses a richer syllabic model without introducing script-specific
    // primitives into the DSL.
    //
    // This is intentionally representative rather than a complete USE
    // specification.
    // ------------------------------------------------------------------------

    static inline bool testRecognitionModelUSE()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef repha = dsl.kind("Repha");
        const ScriptItemKindRef consonant = dsl.kind("Consonant");
        const ScriptItemKindRef halant = dsl.kind("Halant");
        const ScriptItemKindRef medial = dsl.kind("Medial");
        const ScriptItemKindRef vowelPre = dsl.kind("VowelPre");
        const ScriptItemKindRef vowelPost = dsl.kind("VowelPost");
        const ScriptItemKindRef mark = dsl.kind("Mark");

        const ScriptRoleRef rephaRole = dsl.role("Repha");
        const ScriptRoleRef base = dsl.role("Base");
        const ScriptRoleRef preBaseVowels = dsl.role("PreBaseVowels");

        if (!repha || !consonant || !halant || !medial ||
            !vowelPre || !vowelPost || !mark)
        {
            return false;
        }

        if (!rephaRole || !base || !preBaseVowels)
            return false;

        const ScriptExprRef R = dsl.match(repha);
        const ScriptExprRef C = dsl.match(consonant);
        const ScriptExprRef H = dsl.match(halant);
        const ScriptExprRef Medial = dsl.match(medial);
        const ScriptExprRef VPre = dsl.match(vowelPre);
        const ScriptExprRef VPost = dsl.match(vowelPost);
        const ScriptExprRef Mark = dsl.match(mark);

        const ScriptExprRef consonantTail =
            dsl.seq({
                H,
                C
                });

        const ScriptExprRef vowels =
            dsl.choice({
                VPre,
                VPost
                });

        const ScriptExprRef syllable =
            dsl.seq({
                dsl.opt(dsl.capture(rephaRole, R)),
                dsl.capture(base, C),
                dsl.zeroOrMore(consonantTail),
                dsl.zeroOrMore(Medial),
                dsl.opt(
                    dsl.capture(
                        preBaseVowels,
                        dsl.oneOrMore(VPre))),
                dsl.zeroOrMore(vowels),
                dsl.zeroOrMore(Mark)
                });

        if (!syllable)
            return false;

        if (!dsl.unit("USESyllable", syllable))
            return false;

        const ScriptRecognitionDescription& description = dsl.description();

        if (description.kinds.size() != 7)
            return false;

        if (description.roles.size() != 3)
            return false;

        if (description.units.size() != 1)
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::Sequence))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::Choice))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::Optional))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::ZeroOrMore))
            return false;

        if (!descriptionHasExprKind(description, ScriptRecognitionExprKind::OneOrMore))
            return false;

        if (!descriptionHasRoleCapture(description, rephaRole.id))
            return false;

        if (!descriptionHasRoleCapture(description, base.id))
            return false;

        if (!descriptionHasRoleCapture(description, preBaseVowels.id))
            return false;

        return true;
    }


    // ------------------------------------------------------------------------
    // Thai
    //
    // Thai is the useful control case.
    //
    // The scalar operations we have implemented do not require structural
    // recognition. A script can therefore define Kinds for classification or
    // later validation without being forced to define Units or Roles.
    //
    // This verifies that recognition remains optional.
    // ------------------------------------------------------------------------

    static inline bool testRecognitionModelThai()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef consonant = dsl.kind("Consonant");
        const ScriptItemKindRef tone = dsl.kind("Tone");
        const ScriptItemKindRef saraAm = dsl.kind("SaraAm");
        const ScriptItemKindRef nikhahit = dsl.kind("Nikhahit");

        if (!consonant || !tone || !saraAm || !nikhahit)
            return false;

        const ScriptRecognitionDescription& description = dsl.description();

        if (description.kinds.size() != 4)
            return false;

        if (!description.roles.empty())
            return false;

        if (!description.unitTypes.empty())
            return false;

        if (!description.units.empty())
            return false;

        if (!description.empty())
            return false;

        return true;
    }



    static inline bool runScriptRecognitionDSLModels()
    {
        if (!testRecognitionModelDevanagari())
        {
            printf("Script recognition DSL models: FAIL\n");
            printf("  Model: Devanagari\n");
            return false;
        }

        if (!testRecognitionModelArabic())
        {
            printf("Script recognition DSL models: FAIL\n");
            printf("  Model: Arabic\n");
            return false;
        }

        if (!testRecognitionModelMyanmar())
        {
            printf("Script recognition DSL models: FAIL\n");
            printf("  Model: Myanmar\n");
            return false;
        }

        if (!testRecognitionModelHangul())
        {
            printf("Script recognition DSL models: FAIL\n");
            printf("  Model: Hangul\n");
            return false;
        }

        if (!testRecognitionModelEmoji())
        {
            printf("Script recognition DSL models: FAIL\n");
            printf("  Model: Emoji\n");
            return false;
        }

        if (!testRecognitionModelKhmer())
        {
            printf("Script recognition DSL models: FAIL\n");
            printf("  Model: Khmer\n");
            return false;
        }

        if (!testRecognitionModelTibetan())
        {
            printf("Script recognition DSL models: FAIL\n");
            printf("  Model: Tibetan\n");
            return false;
        }

        if (!testRecognitionModelUSE())
        {
            printf("Script recognition DSL models: FAIL\n");
            printf("  Model: USE\n");
            return false;
        }

        if (!testRecognitionModelThai())
        {
            printf("Script recognition DSL models: FAIL\n");
            printf("  Model: Thai\n");
            return false;
        }

        printf(
            "Script recognition DSL models: PASS\n"
            "  Models: 9\n"
            "  Devanagari\n"
            "  Arabic\n"
            "  Myanmar\n"
            "  Hangul\n"
            "  Emoji\n"
            "  Khmer\n"
            "  Tibetan\n"
            "  USE\n"
            "  Thai\n");

        return true;
    }


    static inline void testScriptRecognitionDSLModels()
    {
        runScriptRecognitionDSLModels();
    }

} // namespace waavs