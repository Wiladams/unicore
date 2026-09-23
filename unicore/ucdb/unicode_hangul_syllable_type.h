// unicode_hangul_syllable_type.h

#pragma once

#include <array>
#include <cstdint>
#include <type_traits>


namespace waavs
{
    enum class UnicodeHangulSyllableType : uint8_t
    {
        NotApplicable = 0,

        LeadingJamo = 1,
        VowelJamo = 2,
        TrailingJamo = 3,
        LVSyllable = 4,
        LVTSyllable = 5
    };


    inline constexpr uint8_t kUnicodeHangulSyllableTypeCount = 6;


    inline constexpr std::array<const char*, kUnicodeHangulSyllableTypeCount>
        kUnicodeHangulSyllableTypeNames =
    {
        "NotApplicable",
        "LeadingJamo",
        "VowelJamo",
        "TrailingJamo",
        "LVSyllable",
        "LVTSyllable"
    };


    [[nodiscard]]
    static constexpr bool unicodeHangulSyllableTypeIsValid(uint8_t value) noexcept
    {
        return value < kUnicodeHangulSyllableTypeCount;
    }


    [[nodiscard]]
    static constexpr bool unicodeHangulSyllableTypeIsValid(
        UnicodeHangulSyllableType value) noexcept
    {
        return unicodeHangulSyllableTypeIsValid(
            static_cast<uint8_t>(value));
    }


    [[nodiscard]]
    static constexpr const char* unicodeHangulSyllableTypeName(
        UnicodeHangulSyllableType value) noexcept
    {
        const uint8_t index = static_cast<uint8_t>(value);

        return index < kUnicodeHangulSyllableTypeNames.size()
            ? kUnicodeHangulSyllableTypeNames[index]
            : nullptr;
    }


    static_assert(
        sizeof(UnicodeHangulSyllableType) == 1);

    static_assert(
        std::is_same_v<
        std::underlying_type_t<UnicodeHangulSyllableType>,
        uint8_t>);

    static_assert(
        static_cast<uint8_t>(
            UnicodeHangulSyllableType::NotApplicable) == 0);

    static_assert(
        static_cast<uint8_t>(
            UnicodeHangulSyllableType::LVTSyllable) + 1u ==
        kUnicodeHangulSyllableTypeCount);

    static_assert(
        kUnicodeHangulSyllableTypeNames.size() ==
        kUnicodeHangulSyllableTypeCount);

} // namespace waavs