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



using namespace waavs;

void testScriptShaping()
{
    testScriptShapingIRBuilder();
    testScriptShapingPolicyCompiler();
    testScriptShapingIRExecutor("../../fonts/NotoSans[wdth,wght].ttf");

    testScriptShapingBuffer();

    testScriptShapingIRScalarReplace();

    testScriptShapingIRThaiCmap("../../fonts/NotoSansThai[wdth,wght].ttf");

    testScriptShapingIRThaiReorder();

    testScriptShapingIRScalarMoveLeft();

    testScriptShapingIRScalarMoveLeftExecutor();

    testScriptShapingIRThaiFull("../../fonts/NotoSansThai[wdth,wght].ttf");

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

int main(int argc, char** argv)
{
    //testScriptShaping();
    testScriptRecognition();

    return 0;
}