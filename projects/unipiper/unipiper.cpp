#include "unicode_scalar_stream.h"
#include "unicode_script_analysis.h"
#include "unicode_shaping_run_itemizer.h"

#include "test_utf8_scalar_stream.h"
#include "test_unicode_nfc_stream.h"
#include "test_grapheme_property_stream.h"
#include "test_grapheme_stream.h"
#include "test_grapheme_break_conformance.h"
#include "test_unicode_script_analysis.h"
#include "test_unicode_bidi_analysis.h"
#include "test_unicode_bidi_bd16.h"
#include "test_unicode_bidi_n0_inspection.h"
#include "test_unicode_bidi_n0_resolution.h"
#include "test_unicode_bidi_n0_trailing_nsm.h"
#include "test_unicode_bidi_n0.h"
#include "test_unicode_bidi_n1.h"
#include "test_unicode_bidi_n2.h"
#include "test_unicode_bidi_i1.h"
#include "test_unicode_bidi_i2.h"
#include "test_unicode_shaping_run_itemizer.h"
#include "test_unicode_text_pipeline.h"
#include "test_unicode_text_file_itemization_contextual.h"
#include "test_unicode_shaping_run_context.h"
#include "test_unicode_grapheme_bidi_levels.h"
#include "test_opentype_mark_glyph_sets_view.h"
#include "test_opentype_gsub_ligature_filtering.h"

// Font runs
#include "test_font_support.h"
#include "test_font_run_itemizer.h"

// Shaping
#include "test_opentype_nominal_glyphs.h"
#include "test_opentype_feature_selection.h"
#include "test_opentype_layout_feature_selection.h"
#include "test_opentype_gsub_single_view.h"
#include "test_opentype_gsub_feature_apply.h"
#include "test_opentype_gsub_multiple_view.h"
#include "test_opentype_gsub_multiple_apply.h"
#include "test_opentype_gsub_ligature_view.h"
#include "test_opentype_gsub_ligature_apply.h"
#include "test_opentype_gsub_alternate_view.h"
#include "test_opentype_gsub_alternate_apply.h"
#include "test_opentype_gsub_extension_view.h"
#include "test_opentype_gsub_extension_apply.h"
#include "test_opentype_gdef_view.h"
#include "test_opentype_lookup_glyph_filter.h"

// Layout 5
#include "test_opentype_gsub_context_view.h"


using namespace waavs;



void runTests()
{

    //testUtf8ScalarStream();
    //testUnicodeNfcStream("../../release/unicode-17.0.0.ucdb");
    //testGraphemePropertyStream("../../release/unicode-17.0.0.ucdb");
    //testGraphemeStream();
    //testGraphemeBreakConformance( "../ucdbgen/ucd/auxiliary/GraphemeBreakTest.txt", "../../release/unicode-17.0.0.ucdb");
    //testUnicodeScriptAnalysis("../../release/unicode-17.0.0.ucdb");
    //testUnicodeBidiAnalysis("../../release/unicode-17.0.0.ucdb");
    //testUnicodeBidiBD16("../../release/unicode-17.0.0.ucdb");
    
    // Unicode bidi N0 tests
    //testUnicodeBidiN0Inspection();
    //testUnicodeBidiN0Resolution();
    //testUnicodeBidiN0TrailingNsm();
    //testUnicodeBidiN0("../../release/unicode-17.0.0.ucdb");

    // Unicode bidi N1 tests
    //testUnicodeBidiN1();
    //testUnicodeBidiN2();
    
    // Unicode bidi I1 tests
    //testUnicodeBidiI1();
    //testUnicodeBidiI2();


    // Unicode shaping-run itemizer tests
    //testUnicodeShapingRunItemizer("../../release/unicode-17.0.0.ucdb");
    //testUnicodeTextPipeline("../../release/unicode-17.0.0.ucdb");
    //testUnicodeTextFileItemization("../../release/unicode-17.0.0.ucdb", "./itemization_sample_utf8.txt");
    //testUnicodeTextFileItemizationContextual("../../release/unicode-17.0.0.ucdb", "./itemization_sample_utf8.txt");

    //testUnicodeShapingRunContext("../../release/unicode-17.0.0.ucdb");

    //testUnicodeGraphemeBidiLevels("../../release/unicode-17.0.0.ucdb");

    //testFontSupport("../../release/unicode-17.0.0.ucdb"); 
    //testFontRunItemizer("../../release/unicode-17.0.0.ucdb");   

    // Shaper tests
    //testOpenTypeNominalGlyphs();
    //testOpenTypeFeatureSelection();
    //testOpenTypeLayoutFeatureSelection();
    //testOpenTypeGsubSingleView();
    //testOpenTypeGsubFeatureApply();
    //testOpenTypeGsubMultipleView();
    //testOpenTypeGsubMultipleApply();
    //testOpenTypeGsubLigatureView();
    //testOpenTypeGsubLigatureApply();
    //testOpenTypeGsubAlternateView();
    //testOpenTypeGsubAlternateApply();
    //testOpenTypeGsubExtensionView();
    //testOpenTypeGsubExtensionApply();

    //testOpenTypeGdefView();
    //testOpenTypeMarkGlyphSetsView();
    //testOpenTypeLookupGlyphFilter();
    //testOpenTypeGsubLigatureFiltering();

    testOpenTypeGsubContextView();
}

int main()
{
    runTests();
    return 0;
}
