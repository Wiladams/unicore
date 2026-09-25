#pragma once


#include "test_find_script_fonts.h"
#include "test_report_font_script_coverage.h"
#include "test_report_script_top_fonts.h"
#include "test_cmap_lookup_startup.h"
#include "test_cmap_glyph_index_benchmark.h"


using namespace waavs;

const char* kUnicodeDatabaseFilename = "../resources/unicode.ucdb";

void testFindScripts()
{
    testFindScriptFonts(kUnicodeDatabaseFilename, "c:/windows/fonts", "Hebr");
    testFindScriptFonts(kUnicodeDatabaseFilename, "c:/windows/fonts", "Arab");
    testFindScriptFonts(kUnicodeDatabaseFilename, "c:/windows/fonts", "Deva");
}

void testScriptCoverage()
{
    //testReportFontScriptCoverage(kUnicodeDatabaseFilename, "../resources/fonts", 50.0);
    
    // Show the scripts each font supports, with a minimum coverage threshold.
    //testReportFontScriptCoverage(kUnicodeDatabaseFilename, "c:/windows/fonts", 0.5);
    
    //testReportScriptTopFonts(kUnicodeDatabaseFilename, "c:/windows/fonts");
    //testReportScriptTopFonts(kUnicodeDatabaseFilename, "w:/fonts");

    // Show performance of cmap construction and lookup for a small set of code points.
    //testCmapLookupStartup("c:/windows/fonts");
    //testCmapLookupStartup("../resources/fonts");
    //testCmapLookupStartup("w:/fonts/commonfonts");
    //testCmapLookupStartup("w:/fonts/fonts");
    
    // glyphIndex benchmark: show performance of cmap glyphIndex 
    // lookups for a small set of code points.
    //testCmapGlyphIndexBenchmark("c:/windows/fonts", 10000);
    //testCmapGlyphIndexBenchmark("w:/fonts/fonts", 10000);
    testCmapGlyphIndexBenchmark("w:/fonts/google/fonts", 10000);

}

int main(int argc, char** argv)
{
    //testFindScripts();
    testScriptCoverage();

    return 0;
}