

#include "unicode_database_generator.h"

using namespace waavs;



int createDatabase(int argc, char **argv)
{
    const char* ucdRoot =  argc > 1  ? argv[1]  : "./ucd";


    const char* outputFilename =  argc > 2  ? argv[2]  : "./unicode-17.0.0.ucdb";


    std::printf(
        "Building Unicode database\n"
        "  UCD root: %s\n"
        "  Output:   %s\n\n",
        ucdRoot,
        outputFilename);


    bool res = waavs::buildUnicodeDatabase17( ucdRoot, outputFilename);

    return res ? 0 : 1;
}

int main(int argc, char **argv)
{
    // Database generator
    createDatabase(argc, argv);

    return 0;
}