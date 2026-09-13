// unicode_shaping_run.h

#pragma once

#include <cstdint>

#include "unicode_scalar_stream.h"
#include "unicode_script.h"


namespace waavs
{
    // ========================================================================
    // ShapingCluster
    //
    // Preserve grapheme-cluster boundaries inside a shaping run.
    //
    // scalarOffset is relative to ShapingRunView::scalars.
    //
    // normalizedBegin identifies the first scalar in the normalized scalar
    // stream.
    //
    // source preserves the corresponding provenance envelope in the original
    // UTF-8 source.
    // ========================================================================

    struct ShapingCluster
    {
        uint32_t scalarOffset{ 0 };
        uint32_t scalarCount{ 0 };

        ScalarIndex normalizedBegin{ 0 };
        SourceRange source{};
    };


    // ========================================================================
    // ShapingRunView
    //
    // Borrowed view of one contiguous shaping run.
    //
    // The scalar sequence remains in logical text order. bidiLevel determines
    // the directional interpretation of the run.
    //
    // script is the effective Unicode Script selected for shaping.
    //
    // clusters preserves grapheme boundaries and source provenance. Grapheme
    // boundaries are metadata only; they do not restrict the shaper from
    // performing substitutions or positioning across cluster boundaries.
    //
    // Ownership belongs to the stream or itemizer which produced the view.
    // ========================================================================

    struct ShapingRunView
    {
        const UnicodeScalar* scalars{ nullptr };
        uint32_t scalarCount{ 0 };

        const ShapingCluster* clusters{ nullptr };
        uint32_t clusterCount{ 0 };

        UnicodeScriptIndex script{ kUnicodeScriptIndexInvalid };
        uint8_t bidiLevel{ 0 };

        ScalarIndex normalizedBegin{ 0 };
        SourceRange source{};


        [[nodiscard]] bool empty() const noexcept
        {
            return scalarCount == 0;
        }


        explicit operator bool() const noexcept
        {
            return !empty();
        }


        [[nodiscard]] bool rightToLeft() const noexcept
        {
            return (bidiLevel & 1u) != 0;
        }


        [[nodiscard]] bool leftToRight() const noexcept
        {
            return !rightToLeft();
        }


        [[nodiscard]] const UnicodeScalar* begin() const noexcept
        {
            return scalars;
        }


        [[nodiscard]] const UnicodeScalar* end() const noexcept
        {
            return scalars + scalarCount;
        }


        [[nodiscard]] const ShapingCluster* clusterBegin() const noexcept
        {
            return clusters;
        }


        [[nodiscard]] const ShapingCluster* clusterEnd() const noexcept
        {
            return clusters + clusterCount;
        }
    };
}
