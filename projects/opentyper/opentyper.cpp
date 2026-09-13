


//#include "test_openparser_main.h"
//#include "test_font_system.h"
//#include "test_unicode_coverage.h"

//#include "test_font_directory_filter.h"
//#include "test_opentype_layout_view.h"
//#include "test_opentype_layout_feature_view.h"
//#include "test_opentype_layout_lookup_view.h"

#include "test_opentype_glyf_pathprogram.h"
#include "test_opentype_glyf_pathprogram_gallery.h"
#include "test_svg_glyph_definition_cache.h"
#include "test_svg_text_backend.h"
#include "test_glyph_positioning.h"
#include "test_svg_text_run_adapter.h"
#include "test_gpos_kerning_font_scan.h"
#include "test_svg_latin_text_pipeline.h"
#include "test_svg_text_drawer.h"
#include "test_svg_text_gallery.h"
#include "test_unicode_bidi_visual_order.h"
#include "test_glyph_positioning_rtl.h"
#include "test_horizontal_bidi_positioning.h"
#include "test_svg_bidi_gallery.h"
#include "test_find_hebrew_font.h"




using namespace waavs;

int main()
{
    //testOpenTypeLayoutView();
    //testOpenTypeLayoutFeatureView();
    //testOpenTypeLayoutLookupView();
    //testOpenTypeGlyfPathProgram("C:\\repos\\unicore\\projects\\opentyper\\testdata\\NotoSans[wdth,wght].ttf");
    //testOpenTypeGlyfPathProgramGallery("C:\\repos\\unicore\\projects\\opentyper\\testdata\\NotoSans[wdth,wght].ttf");
    //printf("testSVGGlyphDefinitionCache: %s\n", testSVGGlyphDefinitionCache("w:/fonts/commonfonts") ? "PASS" : "FAIL");
    //printf("testSVGTextBackend: %s\n", testSVGTextBackend("C:\\repos\\unicore\\projects\\opentyper\\testdata\\NotoSans[wdth,wght].ttf") ? "PASS" : "FAIL");

    //testGlyphPositioning();

    //testGposKerningFontScan("w:\\fonts\\commonfonts");

    //testSVGTextRunAdapter("w:\\fonts\\commonfonts\\LiberationSans-Regular.ttf",
    //    ".\\testdata\\test_svg_text_run_adapter.svg");

    //testSVGLatinTextPipeline(
    //    ".\\testdata\\unicode.ucdb",
    //    ".\\testdata\\LiberationSans-Regular.ttf",
    //    ".\\testdata\\test_svg_latin_text_pipeline.svg");

    //testSVGTextDrawer(
    //    ".\\testdata\\unicode.ucdb",
    //    ".\\testdata\\LiberationSans-Regular.ttf",
    //    ".\\testdata\\test_svg_text_drawer.svg");

    //testSVGTextGallery(
    //    ".\\testdata\\unicode.ucdb",
    //    ".\\testdata\\LiberationSans-Regular.ttf",
    //    ".\\testdata\\test_svg_text_gallery.svg");

    //printf("testUnicodeBidiVisualOrder: %s\n", testUnicodeBidiVisualOrder() ? "PASS" : "FAIL");

    //printf("testGlyphPositioningRTL: %s\n", testGlyphPositioningRTL() ? "PASS" : "FAIL");

    printf("testHorizontalBidiPositioning: %s\n", testHorizontalBidiPositioning() ? "PASS" : "FAIL");

    //testFindHebrewFont(".\\testdata\\unicode.ucdb", ".\\testdata");


    //printf("testSVGBidiGallery: %s\n", testSVGBidiGallery(
    //    ".\\testdata\\unicode.ucdb",
    //    ".\\testdata\\NotoSerifHebrew[wdth,wght].ttf",
    //    ".\\testdata\\test_svg_bidi_gallery.svg") ? "PASS" : "FAIL");


}