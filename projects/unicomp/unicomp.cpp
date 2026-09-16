
#include "test_opentype_gsub_ir_single.h"
#include "test_opentype_gsub_ir_multiple.h"
#include "test_opentype_gsub_ir_ligature.h"
#include "test_opentype_gsub_ir_context.h"
#include "test_opentype_gsub_ir_chain_context.h"
#include "test_opentype_gsub_ir_alternate.h"
#include "test_opentype_gsub_ir_reverse_chain_single.h"

using namespace waavs;

int main(int argc, char** argv)
{
    //testOpenTypeGsubIRSingle("../opentyper/testdata/NotoSans[wdth,wght].ttf");
    //testOpenTypeGsubIRMultiple("../opentyper/testdata/NotoSans[wdth,wght].ttf");
    //testOpenTypeGsubIRLigature("../opentyper/testdata/NotoSans[wdth,wght].ttf");
    //testOpenTypeGsubIRContext();
    //testOpenTypeGsubIRChainContext();
    //testOpenTypeGsubIRAlternate();
    testOpenTypeGsubIRReverseChainSingle();
    return 0;
}