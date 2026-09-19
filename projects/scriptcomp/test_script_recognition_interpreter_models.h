// test_script_recognition_interpreter_models.h
#pragma once

#include "test_core.h"

#include "script_recognition_compiler.h"
#include "script_recognition_dsl.h"
#include "script_recognition_interpreter.h"

namespace waavs
{
    static inline bool checkScriptRecognitionUnit(const ScriptRecognitionResult& result, size_t index, uint32_t first, uint32_t count, uint16_t roleCount)
    {
        if (index >= result.units.size())
            return false;

        const ScriptRecognitionUnit& unit = result.units[index];

        return unit.span.first == first &&
            unit.span.count == count &&
            unit.roleCount == roleCount;
    }


    static inline bool checkScriptRecognitionRole(const ScriptRecognitionResult& result, const ScriptRecognitionUnit& unit, ScriptRoleId roleId, uint32_t first, uint32_t count)
    {
        const ScriptRoleBinding* role = result.roleFor(unit, roleId);

        if (!role)
            return false;

        return role->span.first == first && role->span.count == count;
    }


    // ------------------------------------------------------------
    // Devanagari
    // ------------------------------------------------------------

    static inline bool testScriptRecognitionInterpreterModelDevanagari()
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

        const ScriptExprRef Ra = dsl.match(ra);
        const ScriptExprRef H = dsl.match(halant);
        const ScriptExprRef C = dsl.match(consonant);
        const ScriptExprRef N = dsl.match(nukta);
        const ScriptExprRef MPre = dsl.match(matraPre);

        const ScriptExprRef expression =
            dsl.seq({
                dsl.opt(dsl.capture(rephCandidate, dsl.seq({ Ra, H }))),
                dsl.zeroOrMore(dsl.seq({ C, dsl.opt(N), H })),
                dsl.capture(base, C),
                dsl.opt(dsl.capture(preBaseMatra, MPre))
                });

        if (!dsl.unit("ConsonantSyllable", expression))
            return false;

        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;

        const std::vector<ScriptItemKindId> kinds =
        {
            ra.id,
            halant.id,
            consonant.id,
            halant.id,
            consonant.id,
            matraPre.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        if (result.units.size() != 1)
            return false;

        const ScriptRecognitionUnit& unit = result.units[0];

        if (!checkScriptRecognitionUnit(result, 0, 0, 6, 3))
            return false;

        if (!checkScriptRecognitionRole(result, unit, rephCandidate.id, 0, 2))
            return false;

        if (!checkScriptRecognitionRole(result, unit, base.id, 4, 1))
            return false;

        if (!checkScriptRecognitionRole(result, unit, preBaseMatra.id, 5, 1))
            return false;

        return true;
    }


    // ------------------------------------------------------------
    // Arabic
    // ------------------------------------------------------------

    static inline bool testScriptRecognitionInterpreterModelArabic()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef dual = dsl.kind("DualJoining");
        const ScriptItemKindRef right = dsl.kind("RightJoining");
        const ScriptItemKindRef transparent = dsl.kind("Transparent");
        const ScriptItemKindRef causing = dsl.kind("JoinCausing");

        const ScriptExprRef expression =
            dsl.oneOrMore(
                dsl.choice({
                    dsl.match(dual),
                    dsl.match(right),
                    dsl.match(transparent),
                    dsl.match(causing)
                    }));

        if (!dsl.unit("JoiningRun", expression))
            return false;

        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;

        const std::vector<ScriptItemKindId> kinds =
        {
            dual.id,
            transparent.id,
            dual.id,
            causing.id,
            right.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        return result.units.size() == 1 &&
            checkScriptRecognitionUnit(result, 0, 0, 5, 0);
    }


    // ------------------------------------------------------------
    // Myanmar
    // ------------------------------------------------------------

    static inline bool testScriptRecognitionInterpreterModelMyanmar()
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

        const ScriptExprRef expression =
            dsl.seq({
                dsl.opt(dsl.capture(kinzi, dsl.seq({
                    dsl.match(kinziConsonant),
                    dsl.match(asat),
                    dsl.match(virama)
                }))),
                dsl.capture(base, dsl.match(consonant)),
                dsl.opt(dsl.capture(preBaseVowels, dsl.oneOrMore(dsl.match(vowelPre))))
                });

        if (!dsl.unit("Syllable", expression))
            return false;

        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;

        const std::vector<ScriptItemKindId> kinds =
        {
            kinziConsonant.id,
            asat.id,
            virama.id,
            consonant.id,
            vowelPre.id,
            vowelPre.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        if (result.units.size() != 1)
            return false;

        const ScriptRecognitionUnit& unit = result.units[0];

        return checkScriptRecognitionUnit(result, 0, 0, 6, 3) &&
            checkScriptRecognitionRole(result, unit, kinzi.id, 0, 3) &&
            checkScriptRecognitionRole(result, unit, base.id, 3, 1) &&
            checkScriptRecognitionRole(result, unit, preBaseVowels.id, 4, 2);
    }


    // ------------------------------------------------------------
    // Hangul
    // ------------------------------------------------------------

    static inline bool testScriptRecognitionInterpreterModelHangul()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef l = dsl.kind("LeadingJamo");
        const ScriptItemKindRef v = dsl.kind("VowelJamo");
        const ScriptItemKindRef t = dsl.kind("TrailingJamo");

        const ScriptExprRef expression =
            dsl.seq({
                dsl.match(l),
                dsl.match(v),
                dsl.opt(dsl.match(t))
                });

        if (!dsl.unit("JamoSequence", expression))
            return false;

        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;

        const std::vector<ScriptItemKindId> kinds =
        {
            l.id,
            v.id,
            t.id,
            l.id,
            v.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        return result.units.size() == 2 &&
            checkScriptRecognitionUnit(result, 0, 0, 3, 0) &&
            checkScriptRecognitionUnit(result, 1, 3, 2, 0);
    }


    // ------------------------------------------------------------
    // Emoji
    // ------------------------------------------------------------

    static inline bool testScriptRecognitionInterpreterModelEmoji()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef ep = dsl.kind("ExtendedPictographic");
        const ScriptItemKindRef extend = dsl.kind("Extend");
        const ScriptItemKindRef zwj = dsl.kind("ZWJ");

        const ScriptExprRef expression =
            dsl.seq({
                dsl.match(ep),
                dsl.zeroOrMore(dsl.match(extend)),
                dsl.match(zwj),
                dsl.match(ep)
                });

        if (!dsl.unit("EmojiZWJSequence", expression))
            return false;

        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;

        const std::vector<ScriptItemKindId> kinds =
        {
            ep.id,
            extend.id,
            extend.id,
            zwj.id,
            ep.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        return result.units.size() == 1 &&
            checkScriptRecognitionUnit(result, 0, 0, 5, 0);
    }


    // ------------------------------------------------------------
    // Khmer
    // ------------------------------------------------------------

    static inline bool testScriptRecognitionInterpreterModelKhmer()
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

        const ScriptExprRef expression =
            dsl.seq({
                dsl.capture(base, dsl.match(consonant)),
                dsl.zeroOrMore(
                    dsl.choice({
                        dsl.capture(preBaseSubscript, dsl.seq({
                            dsl.match(coeng),
                            dsl.match(ro)
                        })),
                        dsl.seq({
                            dsl.match(coeng),
                            dsl.match(consonant)
                        })
                    })),
                dsl.opt(dsl.capture(preBaseVowel, dsl.match(vowelPre))),
                dsl.zeroOrMore(dsl.match(mark))
                });

        if (!dsl.unit("Syllable", expression))
            return false;

        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;

        const std::vector<ScriptItemKindId> kinds =
        {
            consonant.id,
            coeng.id,
            ro.id,
            vowelPre.id,
            mark.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        if (result.units.size() != 1)
            return false;

        const ScriptRecognitionUnit& unit = result.units[0];

        return checkScriptRecognitionUnit(result, 0, 0, 5, 3) &&
            checkScriptRecognitionRole(result, unit, base.id, 0, 1) &&
            checkScriptRecognitionRole(result, unit, preBaseSubscript.id, 1, 2) &&
            checkScriptRecognitionRole(result, unit, preBaseVowel.id, 3, 1);
    }


    // ------------------------------------------------------------
    // Tibetan
    // ------------------------------------------------------------

    static inline bool testScriptRecognitionInterpreterModelTibetan()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef baseLetter = dsl.kind("BaseLetter");
        const ScriptItemKindRef subjoined = dsl.kind("Subjoined");
        const ScriptItemKindRef vowel = dsl.kind("Vowel");
        const ScriptItemKindRef mark = dsl.kind("Mark");

        const ScriptRoleRef base = dsl.role("Base");
        const ScriptRoleRef subjoinedStack = dsl.role("SubjoinedStack");

        const ScriptExprRef expression =
            dsl.seq({
                dsl.capture(base, dsl.match(baseLetter)),
                dsl.opt(dsl.capture(subjoinedStack, dsl.oneOrMore(dsl.match(subjoined)))),
                dsl.opt(dsl.match(vowel)),
                dsl.zeroOrMore(dsl.match(mark))
                });

        if (!dsl.unit("Stack", expression))
            return false;

        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;

        const std::vector<ScriptItemKindId> kinds =
        {
            baseLetter.id,
            subjoined.id,
            subjoined.id,
            vowel.id,
            mark.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        if (result.units.size() != 1)
            return false;

        const ScriptRecognitionUnit& unit = result.units[0];

        return checkScriptRecognitionUnit(result, 0, 0, 5, 2) &&
            checkScriptRecognitionRole(result, unit, base.id, 0, 1) &&
            checkScriptRecognitionRole(result, unit, subjoinedStack.id, 1, 2);
    }


    // ------------------------------------------------------------
    // USE
    // ------------------------------------------------------------

    static inline bool testScriptRecognitionInterpreterModelUSE()
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

        const ScriptExprRef expression =
            dsl.seq({
                dsl.opt(dsl.capture(rephaRole, dsl.match(repha))),
                dsl.zeroOrMore(dsl.seq({
                    dsl.match(consonant),
                    dsl.match(halant)
                })),
                dsl.capture(base, dsl.match(consonant)),
                dsl.zeroOrMore(dsl.match(medial)),
                dsl.opt(dsl.capture(preBaseVowels, dsl.oneOrMore(dsl.match(vowelPre)))),
                dsl.zeroOrMore(dsl.match(vowelPost)),
                dsl.zeroOrMore(dsl.match(mark))
                });

        if (!dsl.unit("Syllable", expression))
            return false;

        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(dsl.description(), ir))
            return false;

        const std::vector<ScriptItemKindId> kinds =
        {
            repha.id,
            consonant.id,
            halant.id,
            consonant.id,
            medial.id,
            vowelPre.id,
            vowelPre.id,
            vowelPost.id,
            mark.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        if (result.units.size() != 1)
            return false;

        const ScriptRecognitionUnit& unit = result.units[0];

        return checkScriptRecognitionUnit(result, 0, 0, 9, 3) &&
            checkScriptRecognitionRole(result, unit, rephaRole.id, 0, 1) &&
            checkScriptRecognitionRole(result, unit, base.id, 3, 1) &&
            checkScriptRecognitionRole(result, unit, preBaseVowels.id, 5, 2);
    }


    // ------------------------------------------------------------
    // Thai
    //
    // Thai deliberately defines kinds but no recognition unit.
    // Recognition therefore preserves the kind stream but produces no
    // unit or role structure.
    // ------------------------------------------------------------

    static inline bool testScriptRecognitionInterpreterModelThai()
    {
        ScriptRecognitionDSL dsl;

        const ScriptItemKindRef consonant = dsl.kind("Consonant");
        const ScriptItemKindRef tone = dsl.kind("Tone");
        const ScriptItemKindRef saraAm = dsl.kind("SaraAm");
        const ScriptItemKindRef nikhahit = dsl.kind("Nikhahit");

        const ScriptRecognitionDescription& description = dsl.description();

        if (description.kinds.size() != 4 || !description.units.empty())
            return false;

        ScriptRecognitionIR ir;

        if (!compileScriptRecognitionIR(description, ir))
            return false;

        if (!ir.empty())
            return false;

        const std::vector<ScriptItemKindId> kinds =
        {
            consonant.id,
            tone.id,
            saraAm.id,
            nikhahit.id
        };

        ScriptRecognitionResult result;

        if (!recognizeScriptKinds(ir, kinds, result))
            return false;

        return result.kinds == kinds &&
            result.units.empty() &&
            result.roles.empty();
    }


    static inline bool runScriptRecognitionInterpreterModels()
    {
        if (!testScriptRecognitionInterpreterModelDevanagari())
            return false;

        if (!testScriptRecognitionInterpreterModelArabic())
            return false;

        if (!testScriptRecognitionInterpreterModelMyanmar())
            return false;

        if (!testScriptRecognitionInterpreterModelHangul())
            return false;

        if (!testScriptRecognitionInterpreterModelEmoji())
            return false;

        if (!testScriptRecognitionInterpreterModelKhmer())
            return false;

        if (!testScriptRecognitionInterpreterModelTibetan())
            return false;

        if (!testScriptRecognitionInterpreterModelUSE())
            return false;

        if (!testScriptRecognitionInterpreterModelThai())
            return false;

        printf(
            "Script recognition interpreter models: PASS\n"
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


    static inline void testScriptRecognitionInterpreterModels()
    {
        runScriptRecognitionInterpreterModels();
    }

} // namespace waavs