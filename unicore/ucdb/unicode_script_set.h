// unicode_script_set.h

#pragma once

#include <cstdint>

#include "unicode_script.h"

namespace waavs
{
    struct UnicodeScriptSet
    {
        uint64_t bits[4]{};

        void clear() noexcept
        {
            bits[0] = 0;
            bits[1] = 0;
            bits[2] = 0;
            bits[3] = 0;
        }

        void add(UnicodeScriptIndex script) noexcept
        {
            if (script == kUnicodeScriptIndexInvalid)
                return;

            bits[script >> 6] |=
                uint64_t(1) << (script & 63u);
        }

        [[nodiscard]] bool contains(UnicodeScriptIndex script) const noexcept
        {
            if (script == kUnicodeScriptIndexInvalid)
                return false;

            return
                (bits[script >> 6] &
                    (uint64_t(1) << (script & 63u))) != 0;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return
                bits[0] == 0 &&
                bits[1] == 0 &&
                bits[2] == 0 &&
                bits[3] == 0;
        }

        // How many scripts are in this set?
        [[nodiscard]] uint32_t count() const noexcept
        {
            uint32_t result = 0;

            for (uint32_t i = 0; i < 4; ++i)
            {
                uint64_t value = bits[i];

                while (value)
                {
                    value &= value - 1u;
                    ++result;
                }
            }

            return result;
        }

        [[nodiscard]] bool isSingleton() const noexcept
        {
            return count() == 1;
        }

        [[nodiscard]] UnicodeScriptIndex first() const noexcept
        {
            for (uint32_t wordIndex = 0; wordIndex < 4; ++wordIndex)
            {
                uint64_t value = bits[wordIndex];

                if (value == 0)
                    continue;

                uint32_t bitIndex = 0;

                while ((value & 1u) == 0)
                {
                    value >>= 1;
                    ++bitIndex;
                }

                const uint32_t script = wordIndex * 64u + bitIndex;

                if (script >= kUnicodeScriptIndexInvalid)
                    return kUnicodeScriptIndexInvalid;

                return static_cast<UnicodeScriptIndex>(script);
            }

            return kUnicodeScriptIndexInvalid;
        }

        [[nodiscard]] bool intersects(const UnicodeScriptSet& other) const noexcept
        {
            return
                ((bits[0] & other.bits[0]) |
                    (bits[1] & other.bits[1]) |
                    (bits[2] & other.bits[2]) |
                    (bits[3] & other.bits[3])) != 0;
        }

        // Intersect this set with another set, returning a new set that is the intersection of the two.
        [[nodiscard]]
        UnicodeScriptSet intersection(const UnicodeScriptSet& other) const noexcept
        {
            UnicodeScriptSet result{};

            result.bits[0] = bits[0] & other.bits[0];
            result.bits[1] = bits[1] & other.bits[1];
            result.bits[2] = bits[2] & other.bits[2];
            result.bits[3] = bits[3] & other.bits[3];

            return result;
        }

        // Intersect this set with another set, modifying this set in place.
        // Essentially operator&=, but with a more descriptive name.
        UnicodeScriptSet& intersectWith(const UnicodeScriptSet& other) noexcept
        {
            bits[0] &= other.bits[0];
            bits[1] &= other.bits[1];
            bits[2] &= other.bits[2];
            bits[3] &= other.bits[3];

            return *this;
        }


        [[nodiscard]] bool operator==(const UnicodeScriptSet& other) const noexcept
        {
            return
                bits[0] == other.bits[0] &&
                bits[1] == other.bits[1] &&
                bits[2] == other.bits[2] &&
                bits[3] == other.bits[3];
        }

        [[nodiscard]] bool operator!=(const UnicodeScriptSet& other) const noexcept
        {
            return !(*this == other);
        }

        // =====================
        // Factory methods
        // =====================
        // 
        // Create a singleton set containing exactly one script.
        [[nodiscard]] static UnicodeScriptSet singleton(UnicodeScriptIndex script) noexcept
        {
            UnicodeScriptSet result{};
            result.add(script);
            return result;
        }
    };

    static_assert(sizeof(UnicodeScriptSet) == 32,
        "UnicodeScriptSet must be exactly 32 bytes");
}