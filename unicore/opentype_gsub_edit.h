// opentype_gsub_edit.h
#pragma once

#include <cstddef>
#include <vector>


namespace waavs
{
    // ====================================================================
    // OpenTypeGsubEdit
    //
    // Describes one atomic GSUB substitution in physical buffer
    // coordinates immediately BEFORE the substitution is performed.
    //
    // inputPositions:
    //   Physical positions consumed by the substitution, in increasing
    //   order. inputPositions[0] is the anchor position.
    //
    // outputCount:
    //   Number of glyphs replacing the anchor glyph.
    //
    // Examples:
    //
    //   Single:
    //     inputPositions = { 3 }
    //     outputCount = 1
    //
    //   Multiple:
    //     inputPositions = { 3 }
    //     outputCount = 3
    //
    //   Ligature:
    //     inputPositions = { 3, 5, 8 }
    //     outputCount = 1
    //
    // Ignored glyphs between consumed positions are not included.
    // ====================================================================

    struct OpenTypeGsubEdit
    {
        std::vector<size_t> inputPositions{};
        size_t outputCount{ 0 };

        void clear() noexcept
        {
            inputPositions.clear();
            outputCount = 0;
        }

        [[nodiscard]] bool isValid() const noexcept
        {
            if (inputPositions.empty() || outputCount == 0)
                return false;

            for (size_t i = 1; i < inputPositions.size(); ++i)
            {
                if (inputPositions[i] <= inputPositions[i - 1])
                    return false;
            }

            return true;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return isValid(); }
        [[nodiscard]] size_t inputCount() const noexcept { return inputPositions.size(); }
        [[nodiscard]] size_t anchor() const noexcept { return inputPositions.empty() ? 0 : inputPositions[0]; }
    };


    using OpenTypeGsubEditLog = std::vector<OpenTypeGsubEdit>;

} // namespace waavs