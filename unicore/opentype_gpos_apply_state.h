// opentype_gpos_apply_state.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace waavs
{
    // ====================================================================
    // OpenTypeGposApplyAtResult
    //
    // Result of applying one lookup exactly at one physical glyph position.
    //
    // Invalid:
    //   Malformed lookup data or execution failure.
    //
    // NoMatch:
    //   Lookup is valid but does not apply at this position.
    //
    // Match:
    //   Lookup matched. It may or may not have changed placement.
    // ====================================================================

    enum class OpenTypeGposApplyAtResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    // ====================================================================
    // OpenTypeGposApplyRange
    //
    // Physical buffer range available to one contextual nested lookup.
    //
    // The range is half-open:
    //
    //   [begin, end)
    //
    // Outer non-contextual lookup execution does not need a range.
    // A null range pointer means unrestricted traversal.
    //
    // Contextual Type 7:
    //
    //   matched positions = { 2, 4, 6 }
    //
    //   range = [2, 7)
    //
    // Glyphs skipped by the OUTER LookupFlag remain physically inside this
    // range. A nested lookup applies its own LookupFlag while traversing.
    // ====================================================================

    struct OpenTypeGposApplyRange
    {
        size_t begin{ 0 };
        size_t end{ 0 };

        [[nodiscard]] bool isValid() const noexcept
        {
            return begin <= end;
        }

        [[nodiscard]] bool validFor(size_t bufferSize) const noexcept
        {
            return begin <= end && end <= bufferSize;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return begin == end;
        }

        [[nodiscard]] size_t size() const noexcept
        {
            return begin <= end ? end - begin : 0;
        }

        [[nodiscard]] bool contains(size_t index) const noexcept
        {
            return index >= begin && index < end;
        }

        [[nodiscard]] static OpenTypeGposApplyRange whole(size_t bufferSize) noexcept
        {
            return { 0, bufferSize };
        }
    };


    // ====================================================================
    // OpenTypeGposApplyState
    //
    // Recursive contextual lookups may reference other contextual lookups.
    //
    // Keep both a nesting limit and an operation budget so malformed fonts
    // cannot create unbounded lookup recursion or work.
    // ====================================================================

    struct OpenTypeGposApplyState
    {
        static constexpr uint32_t kDefaultMaxNestingDepth = 32;

        uint32_t nestingDepth{ 0 };
        uint32_t maxNestingDepth{ kDefaultMaxNestingDepth };

        size_t operationBudget{
            std::numeric_limits<size_t>::max()
        };

        [[nodiscard]] bool enter() noexcept
        {
            if (nestingDepth >= maxNestingDepth)
                return false;

            ++nestingDepth;
            return true;
        }

        void leave() noexcept
        {
            if (nestingDepth != 0)
                --nestingDepth;
        }

        [[nodiscard]] bool consumeOperation(size_t count = 1) noexcept
        {
            if (count > operationBudget)
                return false;

            operationBudget -= count;
            return true;
        }
    };


    // ====================================================================
    // OpenTypeGposApplyScope
    //
    // RAII recursion-depth guard.
    // ====================================================================

    class OpenTypeGposApplyScope
    {
    public:
        explicit OpenTypeGposApplyScope(OpenTypeGposApplyState& state) noexcept
            : fState(&state)
            , fEntered(state.enter())
        {}

        ~OpenTypeGposApplyScope()
        {
            if (fEntered)
                fState->leave();
        }

        OpenTypeGposApplyScope(const OpenTypeGposApplyScope&) = delete;
        OpenTypeGposApplyScope& operator=(const OpenTypeGposApplyScope&) = delete;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return fEntered;
        }

    private:
        OpenTypeGposApplyState* fState{ nullptr };
        bool fEntered{ false };
    };

} // namespace waavs