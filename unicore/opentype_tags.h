#pragma once

#include <cstdint>

namespace waavs
{
    using Tag = uint32_t;

    // ============================================================================
    // Table Tags (constant values for fast comparison)
    // ============================================================================
    // Construct a tag from a 4-character string literal (big-endian)
    // This one is used at compile time for constant tags, e.g., OTAG("cmap")
    consteval Tag OTAG(const char(&s)[5]) noexcept
    {
        return
            (static_cast<Tag>(static_cast<uint8_t>(s[0])) << 24) |
            (static_cast<Tag>(static_cast<uint8_t>(s[1])) << 16) |
            (static_cast<Tag>(static_cast<uint8_t>(s[2])) << 8) |
            static_cast<Tag>(static_cast<uint8_t>(s[3]));
    }

    // Construct a tag from a pointer to 4 bytes (big-endian)
    // This one is used at runtime when reading tags from a font file.
    // The pointer must point to at least 4 bytes of valid memory.
    constexpr Tag OSIG(const uint8_t* p) noexcept
    {
        return
            (static_cast<Tag>(p[0]) << 24) |
            (static_cast<Tag>(p[1]) << 16) |
            (static_cast<Tag>(p[2]) << 8) |
            static_cast<Tag>(p[3]);
    }


}
