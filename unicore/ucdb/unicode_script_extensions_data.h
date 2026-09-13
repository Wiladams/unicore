// unicode_script_extensions_data.h

#pragma once

#include <cstdint>

#include "unicode_script_set.h"

namespace waavs
{
    using UnicodeScriptSetIndex = uint16_t;

    static constexpr UnicodeScriptSetIndex
        kUnicodeScriptSetIndexInvalid = 0xFFFFu;


    struct UnicodeScriptExtensionRange
    {
        uint32_t first;
        uint32_t last;
        UnicodeScriptSetIndex setIndex;
        uint16_t reserved;
    };

    static_assert(sizeof(UnicodeScriptExtensionRange) == 12,
        "UnicodeScriptExtensionRange must be exactly 12 bytes");
}