#include "test_script_shaping_ir_builder.h"
#include "test_script_shaping_policy_compiler.h"
#include "test_script_shaping_ir_executor.h"
#include "test_script_shaping_buffer.h"
#include "test_script_shaping_ir_scalar_replace.h"
#include "test_script_shaping_ir_thai_cmap.h"
#include "test_script_shaping_ir_thai_reorder.h"
#include "test_script_shaping_ir_scalar_move_left.h"
#include "test_script_shaping_ir_scalar_move_left_executor.h"
#include "test_script_shaping_ir_thai_full.h"
#include "test_script_shaping_ir_thai_conformance.h"

// script recognition
#include "test_script_recognition_types.h"
#include "test_script_recognition_result.h"
#include "test_script_recognition_dsl.h"
#include "test_script_recognition_dsl_models.h"
#include "test_script_recognition_validator.h"
#include "test_script_recognition_dump.h"
#include "test_script_recognition_ir_types.h"
#include "test_script_recognition_ir.h"
#include "test_script_recognition_compiler.h"
#include "test_script_recognition_compiler_control.h"
#include "test_script_recognition_interpreter.h"
#include "test_script_recognition_interpreter_control.h"
#include "test_script_recognition_interpreter_backtracking.h"
#include "test_script_recognition_interpreter_models.h"

// Classifier
#include "test_script_item_classifier_dsl.h"
#include "test_script_item_classifier_validator.h"
#include "test_script_item_classifier_dump.h"
#include "test_script_item_classifier_eval.h"
#include "test_script_item_classifier_ucdb.h"
#include "test_script_item_recognition.h"
#include "test_devanagari_item_classifier_dump.h"
#include "test_item_classifier_thai_dump.h"

#include "test_recognition_devanagari_dump.h"
#include "test_recognition_devanagari_integration.h"
#include "test_script_shaping_provenance.h"
#include "test_script_shaping_selection_types.h"
#include "test_script_shaping_selection.h"
#include "test_script_shaping_selection_assignment.h"
#include "test_script_shaping_ir_selection.h"
#include "test_script_shaping_glyph_selection.h"
#include "test_opentype_gsub_ir_selected.h"
#include "test_script_shaping_ir_gsub_selection.h"
#include "test_script_shaping_ir_gsub_changed_selection.h"
#include "test_shaping_devanagari.h"
#include "test_script_shaping_indic_base.h"
#include "test_script_shaping_indic_base_gsub.h"
#include "test_script_shaping_move_selection.h"
#include "test_script_shaping_indic_reordering.h"
#include "test_shaping_devanagari_end_to_end.h"
#include "test_svg_devanagari_gallery.h"




using namespace waavs;

void testScriptShaping()
{
    testScriptShapingIRBuilder();
    testScriptShapingPolicyCompiler();
    testScriptShapingIRExecutor("../resources/fonts/NotoSans[wdth,wght].ttf");

    testScriptShapingBuffer();

    testScriptShapingIRScalarReplace();

    testScriptShapingIRThaiCmap("../resources/fonts/NotoSansThai[wdth,wght].ttf");

    testScriptShapingIRThaiReorder();

    testScriptShapingIRScalarMoveLeft();

    testScriptShapingIRScalarMoveLeftExecutor();

    testScriptShapingIRThaiFull("../resources/fonts/NotoSansThai[wdth,wght].ttf");

    testScriptShapingIRThaiConformance();
}

void testScriptRecognition()
{
    //testScriptRecognitionTypes();
    //testScriptRecognitionResult();
    //testScriptRecognitionDSL();
    //testScriptRecognitionDSLModels();
    //testScriptRecognitionValidator();
    //testScriptRecognitionDump();

    testScriptRecognitionIRTypes();
    testScriptRecognitionIR();
    testScriptRecognitionCompiler();
    testScriptRecognitionCompilerControl();
    testScriptRecognitionInterpreter();
    testScriptRecognitionInterpreterControl();
    testScriptRecognitionInterpreterBacktracking();
    testScriptRecognitionInterpreterModels();
}

void testScriptItemClassifier()
{
    //testScriptItemClassifierDSL();
    //testScriptItemClassifierValidator();
    //testScriptItemClassifierDump();
    //testScriptItemClassifierEval();
    //testScriptItemClassifierUCDB("../resources/unicode.ucdb");
    //testScriptItemRecognition("../resources/unicode.ucdb");
    //testDevanagariItemClassifierDump();
    //testThaiItemClassifierDump();

    testDevanagariRecognitionDump();
    testDevanagariRecognitionIntegration("../resources/unicode.ucdb");
}

void testScriptIntegration()
{
    testScriptShapingProvenance();
    testScriptShapingSelectionTypes();
    testScriptShapingSelection();
    testScriptShapingSelectionAssignment();
    testScriptShapingIRSelection();
    testScriptShapingGlyphSelection();
    testOpenTypeGsubIRSelected();
    testScriptShapingIRGsubSelection();
    testScriptShapingIRGsubChangedSelection();
    testScriptShapingIndicBase();
    testScriptShapingIndicBaseGsub();
}

void testDevanagari()
{
    //testDevanagariShapingBuilder();
    //testScriptShapingMoveSelection();
    //testScriptShapingIndicReordering();
    //testDevanagariEndToEnd(
    //    "../resources/unicode.ucdb",
    //    "../resources/fonts/NotoSansDevanagari-Regular.ttf");

    testSVGDevanagariGallery(
        "../resources/unicode.ucdb",
        "../resources/fonts/NotoSansDevanagari-Regular.ttf");
}

int main(int argc, char** argv)
{
    //testScriptShaping();
    //testScriptRecognition();
    //testScriptItemClassifier();
    //testScriptIntegration();
    testDevanagari();


    return 0;
}