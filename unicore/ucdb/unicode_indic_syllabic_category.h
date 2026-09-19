// unicode_indic_syllabic_category.h

#pragma once

#include <cstdint>
#include <type_traits>


namespace waavs
{
    // ========================================================================
    // UnicodeIndicSyllabicCategory
    //
    // Unicode Indic_Syllabic_Category property values.
    //
    // These numeric values are part of the persistent Unicode database
    // representation and must remain stable once databases using them are
    // emitted.
    //
    // Other is zero because it is the documented Unicode default.
    //
    // Unicode 17.0.0 defines 37 values.
    // ========================================================================

    enum class UnicodeIndicSyllabicCategory : uint8_t
    {
        Other = 0,

        Avagraha = 1,
        Bindu = 2,
        BrahmiJoiningNumber = 3,
        CantillationMark = 4,

        Consonant = 5,
        ConsonantDead = 6,
        ConsonantFinal = 7,
        ConsonantHeadLetter = 8,
        ConsonantInitialPostfixed = 9,
        ConsonantKiller = 10,
        ConsonantMedial = 11,
        ConsonantPlaceholder = 12,
        ConsonantPrecedingRepha = 13,
        ConsonantPrefixed = 14,
        ConsonantSubjoined = 15,
        ConsonantSucceedingRepha = 16,
        ConsonantWithStacker = 17,

        GeminationMark = 18,
        InvisibleStacker = 19,
        Joiner = 20,
        ModifyingLetter = 21,
        NonJoiner = 22,
        Nukta = 23,

        Number = 24,
        NumberJoiner = 25,

        PureKiller = 26,
        RegisterShifter = 27,
        SyllableModifier = 28,

        ToneLetter = 29,
        ToneMark = 30,

        Virama = 31,
        Visarga = 32,

        Vowel = 33,
        VowelDependent = 34,
        VowelIndependent = 35,

        ReorderingKiller = 36
    };


    static constexpr uint8_t kUnicodeIndicSyllabicCategoryCount = 37;


    [[nodiscard]]
    static constexpr bool unicodeIndicSyllabicCategoryIsValid(uint8_t value) noexcept
    {
        return value < kUnicodeIndicSyllabicCategoryCount;
    }


    [[nodiscard]]
    static constexpr bool unicodeIndicSyllabicCategoryIsValid(
        UnicodeIndicSyllabicCategory value) noexcept
    {
        return unicodeIndicSyllabicCategoryIsValid(
            static_cast<uint8_t>(value));
    }


    static_assert(
        sizeof(UnicodeIndicSyllabicCategory) == 1,
        "UnicodeIndicSyllabicCategory must be exactly one byte");

    static_assert(
        std::is_same<
        std::underlying_type<UnicodeIndicSyllabicCategory>::type,
        uint8_t>::value,
        "UnicodeIndicSyllabicCategory must use uint8_t storage");

    static_assert(
        static_cast<uint8_t>(
            UnicodeIndicSyllabicCategory::Other) == 0,
        "Unicode Indic_Syllabic_Category Other must remain value zero");

    static_assert(
        static_cast<uint8_t>(
            UnicodeIndicSyllabicCategory::ReorderingKiller) + 1u ==
        kUnicodeIndicSyllabicCategoryCount,
        "Unicode Indic_Syllabic_Category count is inconsistent");

} // namespace waavs