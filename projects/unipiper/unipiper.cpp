#include "unicode_scalar_stream.h"
#include "unicode_script_analysis.h"

#include "test_utf8_scalar_stream.h"
#include "test_unicode_nfc_stream.h"
#include "test_grapheme_property_stream.h"
#include "test_grapheme_stream.h"
#include "test_grapheme_break_conformance.h"
#include "test_unicode_script_analysis.h"
#include "test_unicode_bidi_analysis.h"


using namespace waavs;



void runTests()
{

    //testUtf8ScalarStream();
    //testUnicodeNfcStream("../../release/unicode-17.0.0.ucdb");
    //testGraphemePropertyStream("../../release/unicode-17.0.0.ucdb");
    //testGraphemeStream();
    //testGraphemeBreakConformance( "../ucdbgen/ucd/auxiliary/GraphemeBreakTest.txt", "../../release/unicode-17.0.0.ucdb");
    //testUnicodeScriptAnalysis("../../release/unicode-17.0.0.ucdb");
    testUnicodeBidiAnalysis("../../release/unicode-17.0.0.ucdb");
}

int main()
{
    runTests();
    return 0;
}
