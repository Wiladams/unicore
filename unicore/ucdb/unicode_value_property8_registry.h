// unicode_value_property8_registry.h

#pragma once

#include <cstdint>

#include "unicode_database_format.h"
#include "unicode_value_table8_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodeValueProperty8Registry
    //
    // Runtime mapping:
    //
    //      UnicodeValueProperty8
    //          ->
    //      UnicodeValueTable8Index
    //
    // Property zero is reserved for UnicodeValueProperty8Unknown and is never
    // considered present.
    //
    // This is runtime routing state only. It is not part of the persistent
    // UCDB representation.
    // ========================================================================

    struct UnicodeValueProperty8Registry
    {
        UnicodeValueTable8Index tables[
            static_cast<uint32_t>(UnicodeValueProperty8MAX) + 1u];


        constexpr UnicodeValueProperty8Registry() noexcept
            : tables{}
        {
            clear();
        }


        void clear() noexcept
        {
            for (uint32_t i = 0;
                i <= static_cast<uint32_t>(UnicodeValueProperty8MAX);
                ++i)
            {
                tables[i] = kUnicodeValueTable8IndexInvalid;
            }
        }


        [[nodiscard]]
        bool has(UnicodeValueProperty8 property) const noexcept
        {
            const uint32_t index =
                static_cast<uint32_t>(property);

            return index !=
                static_cast<uint32_t>(UnicodeValueProperty8Unknown) &&
                index <= static_cast<uint32_t>(UnicodeValueProperty8MAX) &&
                tables[index] != kUnicodeValueTable8IndexInvalid;
        }


        [[nodiscard]]
        UnicodeValueTable8Index get(
            UnicodeValueProperty8 property) const noexcept
        {
            const uint32_t index =
                static_cast<uint32_t>(property);

            if (index == static_cast<uint32_t>(
                UnicodeValueProperty8Unknown) ||
                index > static_cast<uint32_t>(
                    UnicodeValueProperty8MAX))
            {
                return kUnicodeValueTable8IndexInvalid;
            }

            return tables[index];
        }


        bool set(UnicodeValueProperty8 property, UnicodeValueTable8Index tableIndex) noexcept
        {
            const uint32_t index = static_cast<uint32_t>(property);

            if (index == static_cast<uint32_t>(
                UnicodeValueProperty8Unknown) ||
                index > static_cast<uint32_t>(
                    UnicodeValueProperty8MAX))
            {
                return false;
            }

            if (tables[index] != kUnicodeValueTable8IndexInvalid)
                return false;

            tables[index] = tableIndex;
            return true;
        }
    };

} // namespace waavs