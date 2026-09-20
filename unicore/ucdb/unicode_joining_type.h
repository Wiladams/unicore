// unicode_joining_type.h

#pragma once

#include <cstdint>
#include <type_traits>


namespace waavs
{
    enum class UnicodeJoiningType : uint8_t
    {
        NonJoining = 0,

        JoinCausing = 1,
        DualJoining = 2,
        LeftJoining = 3,
        RightJoining = 4,
        Transparent = 5
    };


    inline constexpr uint8_t kUnicodeJoiningTypeCount = 6;


    static_assert(
        sizeof(UnicodeJoiningType) == 1);

    static_assert(
        std::is_same_v<
        std::underlying_type_t<UnicodeJoiningType>,
        uint8_t>);

    static_assert(
        static_cast<uint8_t>(
            UnicodeJoiningType::NonJoining) == 0);

    static_assert(
        static_cast<uint8_t>(
            UnicodeJoiningType::Transparent) + 1u ==
        kUnicodeJoiningTypeCount);

} // namespace waavs