#include "test_unicode_database_indic_syllabic_category.h"
#include "test_unicode_database_shaping_properties.h"


using namespace waavs;

int main(int argc, char** argv)
{
    testUnicodeDatabaseIndicSyllabicCategory(
        "../resources/unicode.ucdb",
        "../resources/ucd/IndicSyllabicCategory.txt");

    testUnicodeDatabaseShapingProperties(
        "../resources/unicode.ucdb",
        "../resources/ucd/IndicPositionalCategory.txt",
        "../resources/ucd/extracted/DerivedJoiningType.txt",
        "../resources/ucd/extracted/DerivedJoiningGroup.txt",
        "../resources/ucd/HangulSyllableType.txt");
}