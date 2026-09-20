// unicode_indic_positional_category.h

#pragma once

#include <cstdint>
#include <type_traits>


namespace waavs
{
    enum class UnicodeIndicPositionalCategory : uint8_t
    {
        NotApplicable = 0,

        Bottom = 1,
        BottomAndLeft = 2,
        BottomAndRight = 3,
        Left = 4,
        LeftAndRight = 5,
        Overstruck = 6,
        Right = 7,
        Top = 8,
        TopAndBottom = 9,
        TopAndBottomAndLeft = 10,
        TopAndBottomAndRight = 11,
        TopAndLeft = 12,
        TopAndLeftAndRight = 13,
        TopAndRight = 14,
        VisualOrderLeft = 15
    };


    inline constexpr uint8_t kUnicodeIndicPositionalCategoryCount = 16;


    static_assert(
        sizeof(UnicodeIndicPositionalCategory) == 1);

    static_assert(
        std::is_same_v<
        std::underlying_type_t<UnicodeIndicPositionalCategory>,
        uint8_t>);

    static_assert(
        static_cast<uint8_t>(
            UnicodeIndicPositionalCategory::NotApplicable) == 0);

    static_assert(
        static_cast<uint8_t>(
            UnicodeIndicPositionalCategory::VisualOrderLeft) + 1u ==
        kUnicodeIndicPositionalCategoryCount);

} // namespace waavs
