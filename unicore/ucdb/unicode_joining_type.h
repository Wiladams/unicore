// unicode_joining_type.h

#pragma once

#include <array>
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


    inline constexpr std::array<const char*, kUnicodeJoiningTypeCount>
        kUnicodeJoiningTypeNames =
    {
        "NonJoining",
        "JoinCausing",
        "DualJoining",
        "LeftJoining",
        "RightJoining",
        "Transparent"
    };


    [[nodiscard]]
    static constexpr bool unicodeJoiningTypeIsValid(uint8_t value) noexcept
    {
        return value < kUnicodeJoiningTypeCount;
    }


    [[nodiscard]]
    static constexpr bool unicodeJoiningTypeIsValid(
        UnicodeJoiningType value) noexcept
    {
        return unicodeJoiningTypeIsValid(
            static_cast<uint8_t>(value));
    }


    [[nodiscard]]
    static constexpr const char* unicodeJoiningTypeName(
        UnicodeJoiningType value) noexcept
    {
        const uint8_t index = static_cast<uint8_t>(value);

        return index < kUnicodeJoiningTypeNames.size()
            ? kUnicodeJoiningTypeNames[index]
            : nullptr;
    }


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

    static_assert(
        kUnicodeJoiningTypeNames.size() ==
        kUnicodeJoiningTypeCount);

} // namespace waavs