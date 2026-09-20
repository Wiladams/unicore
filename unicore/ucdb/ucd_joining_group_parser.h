// ucd_joining_group_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_joining_group.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    enum class UCDJoiningGroupParseError : uint8_t
    {
        None = 0,

        InvalidRange,
        MissingValue,
        UnexpectedField,
        UnknownValue,
        OverlappingRange,
        NoPropertyData
    };


    struct UCDJoiningGroupParseResult
    {
        UCDJoiningGroupParseError error{
            UCDJoiningGroupParseError::None
        };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };

        size_t explicitCodePoints{ 0 };
        size_t defaultedCodePoints{ 0 };
    };


    static inline const char* ucdJoiningGroupParseErrorString(
        UCDJoiningGroupParseError error) noexcept
    {
        switch (error)
        {
        case UCDJoiningGroupParseError::None:
            return "none";

        case UCDJoiningGroupParseError::InvalidRange:
            return "invalid Joining_Group code-point range";

        case UCDJoiningGroupParseError::MissingValue:
            return "missing Joining_Group value";

        case UCDJoiningGroupParseError::UnexpectedField:
            return "unexpected extra field in Joining_Group record";

        case UCDJoiningGroupParseError::UnknownValue:
            return "unknown Joining_Group value";

        case UCDJoiningGroupParseError::OverlappingRange:
            return "Joining_Group ranges overlap";

        case UCDJoiningGroupParseError::NoPropertyData:
            return "no Joining_Group data found";
        }

        return "unknown Joining_Group parser error";
    }


    namespace ucd_joining_group_detail
    {
        static inline bool spanEquals(
            const ByteSpan& span,
            const char* text) noexcept
        {
            const size_t length = std::strlen(text);

            return span.size() == length &&
                std::memcmp(span.data(), text, length) == 0;
        }


        struct NameRecord
        {
            const char* name;
            UnicodeJoiningGroup value;
        };


        static constexpr NameRecord kNames[] =
        {
            { "No_Joining_Group", UnicodeJoiningGroup::NoJoiningGroup },

            { "African_Feh", UnicodeJoiningGroup::AfricanFeh },
            { "African_Noon", UnicodeJoiningGroup::AfricanNoon },
            { "African_Qaf", UnicodeJoiningGroup::AfricanQaf },
            { "Ain", UnicodeJoiningGroup::Ain },
            { "Alaph", UnicodeJoiningGroup::Alaph },
            { "Alef", UnicodeJoiningGroup::Alef },
            { "Beh", UnicodeJoiningGroup::Beh },
            { "Beth", UnicodeJoiningGroup::Beth },
            { "Burushaski_Yeh_Barree", UnicodeJoiningGroup::BurushaskiYehBarree },
            { "Dal", UnicodeJoiningGroup::Dal },
            { "Dalath_Rish", UnicodeJoiningGroup::DalathRish },
            { "E", UnicodeJoiningGroup::E },
            { "Farsi_Yeh", UnicodeJoiningGroup::FarsiYeh },
            { "Fe", UnicodeJoiningGroup::Fe },
            { "Feh", UnicodeJoiningGroup::Feh },
            { "Final_Semkath", UnicodeJoiningGroup::FinalSemkath },
            { "Gaf", UnicodeJoiningGroup::Gaf },
            { "Gamal", UnicodeJoiningGroup::Gamal },
            { "Hah", UnicodeJoiningGroup::Hah },
            { "Hanifi_Rohingya_Kinna_Ya", UnicodeJoiningGroup::HanifiRohingyaKinnaYa },
            { "Hanifi_Rohingya_Pa", UnicodeJoiningGroup::HanifiRohingyaPa },
            { "He", UnicodeJoiningGroup::He },
            { "Heh", UnicodeJoiningGroup::Heh },
            { "Heh_Goal", UnicodeJoiningGroup::HehGoal },
            { "Heth", UnicodeJoiningGroup::Heth },
            { "Kaf", UnicodeJoiningGroup::Kaf },
            { "Kaph", UnicodeJoiningGroup::Kaph },
            { "Kashmiri_Yeh", UnicodeJoiningGroup::KashmiriYeh },
            { "Khaph", UnicodeJoiningGroup::Khaph },
            { "Knotted_Heh", UnicodeJoiningGroup::KnottedHeh },
            { "Lam", UnicodeJoiningGroup::Lam },
            { "Lamadh", UnicodeJoiningGroup::Lamadh },

            { "Malayalam_Bha", UnicodeJoiningGroup::MalayalamBha },
            { "Malayalam_Ja", UnicodeJoiningGroup::MalayalamJa },
            { "Malayalam_Lla", UnicodeJoiningGroup::MalayalamLla },
            { "Malayalam_Llla", UnicodeJoiningGroup::MalayalamLlla },
            { "Malayalam_Nga", UnicodeJoiningGroup::MalayalamNga },
            { "Malayalam_Nna", UnicodeJoiningGroup::MalayalamNna },
            { "Malayalam_Nnna", UnicodeJoiningGroup::MalayalamNnna },
            { "Malayalam_Nya", UnicodeJoiningGroup::MalayalamNya },
            { "Malayalam_Ra", UnicodeJoiningGroup::MalayalamRa },
            { "Malayalam_Ssa", UnicodeJoiningGroup::MalayalamSsa },
            { "Malayalam_Tta", UnicodeJoiningGroup::MalayalamTta },

            { "Manichaean_Aleph", UnicodeJoiningGroup::ManichaeanAleph },
            { "Manichaean_Ayin", UnicodeJoiningGroup::ManichaeanAyin },
            { "Manichaean_Beth", UnicodeJoiningGroup::ManichaeanBeth },
            { "Manichaean_Daleth", UnicodeJoiningGroup::ManichaeanDaleth },
            { "Manichaean_Dhamedh", UnicodeJoiningGroup::ManichaeanDhamedh },
            { "Manichaean_Five", UnicodeJoiningGroup::ManichaeanFive },
            { "Manichaean_Gimel", UnicodeJoiningGroup::ManichaeanGimel },
            { "Manichaean_Heth", UnicodeJoiningGroup::ManichaeanHeth },
            { "Manichaean_Hundred", UnicodeJoiningGroup::ManichaeanHundred },
            { "Manichaean_Kaph", UnicodeJoiningGroup::ManichaeanKaph },
            { "Manichaean_Lamedh", UnicodeJoiningGroup::ManichaeanLamedh },
            { "Manichaean_Mem", UnicodeJoiningGroup::ManichaeanMem },
            { "Manichaean_Nun", UnicodeJoiningGroup::ManichaeanNun },
            { "Manichaean_One", UnicodeJoiningGroup::ManichaeanOne },
            { "Manichaean_Pe", UnicodeJoiningGroup::ManichaeanPe },
            { "Manichaean_Qoph", UnicodeJoiningGroup::ManichaeanQoph },
            { "Manichaean_Resh", UnicodeJoiningGroup::ManichaeanResh },
            { "Manichaean_Sadhe", UnicodeJoiningGroup::ManichaeanSadhe },
            { "Manichaean_Samekh", UnicodeJoiningGroup::ManichaeanSamekh },
            { "Manichaean_Taw", UnicodeJoiningGroup::ManichaeanTaw },
            { "Manichaean_Ten", UnicodeJoiningGroup::ManichaeanTen },
            { "Manichaean_Teth", UnicodeJoiningGroup::ManichaeanTeth },
            { "Manichaean_Thamedh", UnicodeJoiningGroup::ManichaeanThamedh },
            { "Manichaean_Twenty", UnicodeJoiningGroup::ManichaeanTwenty },
            { "Manichaean_Waw", UnicodeJoiningGroup::ManichaeanWaw },
            { "Manichaean_Yodh", UnicodeJoiningGroup::ManichaeanYodh },
            { "Manichaean_Zayin", UnicodeJoiningGroup::ManichaeanZayin },

            { "Meem", UnicodeJoiningGroup::Meem },
            { "Mim", UnicodeJoiningGroup::Mim },
            { "Noon", UnicodeJoiningGroup::Noon },
            { "Nun", UnicodeJoiningGroup::Nun },
            { "Nya", UnicodeJoiningGroup::Nya },
            { "Pe", UnicodeJoiningGroup::Pe },
            { "Qaf", UnicodeJoiningGroup::Qaf },
            { "Qaph", UnicodeJoiningGroup::Qaph },
            { "Reh", UnicodeJoiningGroup::Reh },
            { "Reversed_Pe", UnicodeJoiningGroup::ReversedPe },
            { "Rohingya_Yeh", UnicodeJoiningGroup::RohingyaYeh },
            { "Sad", UnicodeJoiningGroup::Sad },
            { "Sadhe", UnicodeJoiningGroup::Sadhe },
            { "Seen", UnicodeJoiningGroup::Seen },
            { "Semkath", UnicodeJoiningGroup::Semkath },
            { "Shin", UnicodeJoiningGroup::Shin },
            { "Straight_Waw", UnicodeJoiningGroup::StraightWaw },
            { "Swash_Kaf", UnicodeJoiningGroup::SwashKaf },
            { "Syriac_Waw", UnicodeJoiningGroup::SyriacWaw },
            { "Tah", UnicodeJoiningGroup::Tah },
            { "Taw", UnicodeJoiningGroup::Taw },
            { "Teh_Marbuta", UnicodeJoiningGroup::TehMarbuta },
            { "Teh_Marbuta_Goal", UnicodeJoiningGroup::TehMarbutaGoal },
            { "Teth", UnicodeJoiningGroup::Teth },
            { "Thin_Noon", UnicodeJoiningGroup::ThinNoon },
            { "Thin_Yeh", UnicodeJoiningGroup::ThinYeh },
            { "Vertical_Tail", UnicodeJoiningGroup::VerticalTail },
            { "Waw", UnicodeJoiningGroup::Waw },
            { "Yeh", UnicodeJoiningGroup::Yeh },
            { "Yeh_Barree", UnicodeJoiningGroup::YehBarree },
            { "Yeh_With_Tail", UnicodeJoiningGroup::YehWithTail },
            { "Yudh", UnicodeJoiningGroup::Yudh },
            { "Yudh_He", UnicodeJoiningGroup::YudhHe },
            { "Zain", UnicodeJoiningGroup::Zain },
            { "Zhain", UnicodeJoiningGroup::Zhain },

            { "BAA", UnicodeJoiningGroup::Baa },
            { "FA", UnicodeJoiningGroup::Fa },
            { "HAA", UnicodeJoiningGroup::Haa },
            { "HA_GOAL", UnicodeJoiningGroup::HaGoal },
            { "HA", UnicodeJoiningGroup::Ha },
            { "CAF", UnicodeJoiningGroup::Caf },
            { "KNOTTED_HA", UnicodeJoiningGroup::KnottedHa },
            { "RA", UnicodeJoiningGroup::Ra },
            { "SWASH_CAF", UnicodeJoiningGroup::SwashCaf },
            { "HAMZAH_ON_HA_GOAL", UnicodeJoiningGroup::HamzahOnHaGoal },
            { "TAA_MARBUTAH", UnicodeJoiningGroup::TaaMarbutah },
            { "YA_BARREE", UnicodeJoiningGroup::YaBarree },
            { "YA", UnicodeJoiningGroup::Ya },
            { "ALEF_MAQSURAH", UnicodeJoiningGroup::AlefMaqsurah }
        };


        static_assert(
            sizeof(kNames) / sizeof(kNames[0]) ==
            kUnicodeJoiningGroupCount);


        static inline bool valueFromField(
            const ByteSpan& field,
            UnicodeJoiningGroup& outValue) noexcept
        {
            for (const NameRecord& record : kNames)
            {
                if (spanEquals(field, record.name))
                {
                    outValue = record.value;
                    return true;
                }
            }

            return false;
        }


        static inline bool coverageIntersectsRange(
            const UnicodeCoverageBuilder& coverage,
            uint32_t first,
            uint32_t last) noexcept
        {
            for (uint32_t cp = first;; ++cp)
            {
                if (coverage.contains(cp))
                    return true;

                if (cp == last)
                    break;
            }

            return false;
        }

    } // namespace ucd_joining_group_detail


    // ========================================================================
    // ucdParseJoiningGroup
    //
    // Parse extracted/DerivedJoiningGroup.txt.
    //
    // Records:
    //
    //      code-point-range ; value
    //
    // Unlisted code points default to No_Joining_Group.
    // ========================================================================

    static inline bool ucdParseJoiningGroup(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values,
        UCDJoiningGroupParseResult& outResult)
    {
        outResult = {};

        values.clear(
            static_cast<uint8_t>(
                UnicodeJoiningGroup::NoJoiningGroup));


        UnicodeCoverageBuilder assignedCoverage;

        UCDParser parser(source);
        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields = line.data;


            ByteSpan rangeField;

            if (!ucdReadField(fields, rangeField))
            {
                outResult.error =
                    UCDJoiningGroupParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            UCDCodePointRange range;

            if (!ucdParseCodePointRange(
                rangeField,
                range))
            {
                outResult.error =
                    UCDJoiningGroupParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            ByteSpan valueField;

            if (!ucdReadField(fields, valueField))
            {
                outResult.error =
                    UCDJoiningGroupParseError::MissingValue;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            bspan_trim_spaces(valueField);

            if (!valueField)
            {
                outResult.error =
                    UCDJoiningGroupParseError::MissingValue;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            bspan_trim_spaces(fields);

            if (fields)
            {
                outResult.error =
                    UCDJoiningGroupParseError::UnexpectedField;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            UnicodeJoiningGroup value;

            if (!ucd_joining_group_detail::valueFromField(
                valueField,
                value))
            {
                outResult.error =
                    UCDJoiningGroupParseError::UnknownValue;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            if (ucd_joining_group_detail::coverageIntersectsRange(
                assignedCoverage,
                range.first,
                range.last))
            {
                outResult.error =
                    UCDJoiningGroupParseError::OverlappingRange;

                outResult.lineNumber = line.lineNumber;
                return false;
            }


            values.setRange(
                range.first,
                range.last,
                static_cast<uint8_t>(value));

            assignedCoverage.addRange(
                range.first,
                range.last);


            ++outResult.rangeCount;

            outResult.explicitCodePoints +=
                static_cast<size_t>(
                    range.last - range.first + 1u);
        }


        if (outResult.rangeCount == 0)
        {
            outResult.error =
                UCDJoiningGroupParseError::NoPropertyData;

            return false;
        }


        outResult.defaultedCodePoints =
            static_cast<size_t>(kUnicodeLimit) -
            outResult.explicitCodePoints;


        return true;
    }


    static inline bool ucdParseJoiningGroup(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values)
    {
        UCDJoiningGroupParseResult result;

        return ucdParseJoiningGroup(
            source,
            values,
            result);
    }

} // namespace waavs