#if defined(_WIN32)
#include "win32_read_only_mapped_file.h"
#else
#include "read_only_mapped_file_posix.h"
#endif