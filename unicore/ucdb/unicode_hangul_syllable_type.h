// unicode_hangul_syllable_type.h

#pragma once

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

} // namespace waavs