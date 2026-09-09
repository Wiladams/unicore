// opentype_gsub_apply_state.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>


namespace waavs
{
    // ====================================================================
    // OpenTypeGsubApplyAtResult
    //
    // Match is deliberately distinct from "buffer changed".
    //
    // A contextual lookup may successfully match while containing zero
    // SequenceLookup records, or its nested lookups may produce no edits.
    // ====================================================================

    enum class OpenTypeGsubApplyAtResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };


    // ====================================================================
    // OpenTypeGsubApplyState
    //
    // Shared state for recursively applied GSUB lookups.
    //
    // nestingDepth:
    //   Protects against recursive contextual lookup cycles.
    //
    // operationBudget:
    //   Optional work budget. Unlimited by default. Carrying it here now
    //   means a bounded policy can later be enabled without changing the
    //   nested lookup API.
    // ====================================================================

    struct OpenTypeGsubApplyState
    {
        static constexpr uint32_t kDefaultMaxNestingDepth = 32;

        uint32_t nestingDepth{ 0 };
        uint32_t maxNestingDepth{ kDefaultMaxNestingDepth };
        size_t operationBudget{ std::numeric_limits<size_t>::max() };

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
    // OpenTypeGsubApplyScope
    //
    // RAII nesting guard so early returns cannot leak nesting depth.
    // ====================================================================

    class OpenTypeGsubApplyScope
    {
    public:
        explicit OpenTypeGsubApplyScope(OpenTypeGsubApplyState& state) noexcept
            : fState(&state), fEntered(state.enter())
        {}

        OpenTypeGsubApplyScope(const OpenTypeGsubApplyScope&) = delete;
        OpenTypeGsubApplyScope& operator=(const OpenTypeGsubApplyScope&) = delete;

        ~OpenTypeGsubApplyScope()
        {
            if (fEntered)
                fState->leave();
        }

        [[nodiscard]] explicit operator bool() const noexcept { return fEntered; }

    private:
        OpenTypeGsubApplyState* fState{ nullptr };
        bool fEntered{ false };
    };

} // namespace waavs