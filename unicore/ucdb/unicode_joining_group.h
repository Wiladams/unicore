// unicode_joining_group.h

#pragma once

#include <cstdint>
#include <type_traits>


namespace waavs
{
    enum class UnicodeJoiningGroup : uint8_t
    {
        NoJoiningGroup = 0,

        AfricanFeh,
        AfricanNoon,
        AfricanQaf,
        Ain,
        Alaph,
        Alef,
        Beh,
        Beth,
        BurushaskiYehBarree,
        Dal,
        DalathRish,
        E,
        FarsiYeh,
        Fe,
        Feh,
        FinalSemkath,
        Gaf,
        Gamal,
        Hah,
        HanifiRohingyaKinnaYa,
        HanifiRohingyaPa,
        He,
        Heh,
        HehGoal,
        Heth,
        Kaf,
        Kaph,
        KashmiriYeh,
        Khaph,
        KnottedHeh,
        Lam,
        Lamadh,
        MalayalamBha,
        MalayalamJa,
        MalayalamLla,
        MalayalamLlla,
        MalayalamNga,
        MalayalamNna,
        MalayalamNnna,
        MalayalamNya,
        MalayalamRa,
        MalayalamSsa,
        MalayalamTta,
        ManichaeanAleph,
        ManichaeanAyin,
        ManichaeanBeth,
        ManichaeanDaleth,
        ManichaeanDhamedh,
        ManichaeanFive,
        ManichaeanGimel,
        ManichaeanHeth,
        ManichaeanHundred,
        ManichaeanKaph,
        ManichaeanLamedh,
        ManichaeanMem,
        ManichaeanNun,
        ManichaeanOne,
        ManichaeanPe,
        ManichaeanQoph,
        ManichaeanResh,
        ManichaeanSadhe,
        ManichaeanSamekh,
        ManichaeanTaw,
        ManichaeanTen,
        ManichaeanTeth,
        ManichaeanThamedh,
        ManichaeanTwenty,
        ManichaeanWaw,
        ManichaeanYodh,
        ManichaeanZayin,
        Meem,
        Mim,
        Noon,
        Nun,
        Nya,
        Pe,
        Qaf,
        Qaph,
        Reh,
        ReversedPe,
        RohingyaYeh,
        Sad,
        Sadhe,
        Seen,
        Semkath,
        Shin,
        StraightWaw,
        SwashKaf,
        SyriacWaw,
        Tah,
        Taw,
        TehMarbuta,
        TehMarbutaGoal,
        Teth,
        ThinNoon,
        ThinYeh,
        VerticalTail,
        Waw,
        Yeh,
        YehBarree,
        YehWithTail,
        Yudh,
        YudhHe,
        Zain,
        Zhain,

        Baa,
        Fa,
        Haa,
        HaGoal,
        Ha,
        Caf,
        KnottedHa,
        Ra,
        SwashCaf,
        HamzahOnHaGoal,
        TaaMarbutah,
        YaBarree,
        Ya,
        AlefMaqsurah,

        Count
    };


    inline constexpr uint8_t kUnicodeJoiningGroupCount =
        static_cast<uint8_t>(
            UnicodeJoiningGroup::Count);


    static_assert(
        sizeof(UnicodeJoiningGroup) == 1);

    static_assert(
        std::is_same_v<
        std::underlying_type_t<UnicodeJoiningGroup>,
        uint8_t>);

    static_assert(
        static_cast<uint8_t>(
            UnicodeJoiningGroup::NoJoiningGroup) == 0);

    static_assert(
        kUnicodeJoiningGroupCount == 120);

} // namespace waavs