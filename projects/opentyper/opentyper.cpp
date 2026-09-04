

//#include "test_openparser_main.h"
//#include "test_font_system.h"
//#include "test_unicode_coverage.h"

//#include "test_font_directory_filter.h"
#include "test_opentype_layout_view.h"
#include "test_opentype_layout_feature_view.h"
#include "test_opentype_layout_lookup_view.h"


using namespace waavs;

int main()
{
    testOpenTypeLayoutView();
    testOpenTypeLayoutFeatureView();
    testOpenTypeLayoutLookupView();
}