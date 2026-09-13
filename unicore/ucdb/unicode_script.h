// unicode_script.h

#pragma once

#include <cstdint>

namespace waavs
{
    using UnicodeScriptIndex = uint8_t;

    static constexpr UnicodeScriptIndex kUnicodeScriptIndexInvalid = 0xFFu;

    static_assert(sizeof(UnicodeScriptIndex) == 1, "UnicodeScriptIndex must be exactly one byte");
}