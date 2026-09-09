// font_run.h

#pragma once

#include <cstdint>

#include "font_face.h"
#include "unicode_shaping_run.h"


namespace waavs
{
    // ========================================================================
    // FontRunView
    //
    // One contiguous portion of a ShapingRunView assigned to a single
    // FontFace.
    //
    // Invariants:
    //
    //      - scalars remain in logical order
    //      - script is constant
    //      - bidiLevel is constant
    //      - face is constant
    //      - boundaries occur only between ShapingCluster records
    //      - cluster.scalarOffset is relative to this FontRunView::scalars
    //
    // completeCoverage is false when no candidate face fully covers one or
    // more clusters in this run. The run is still valid and may subsequently
    // produce .notdef glyphs.
    //
    // Ownership remains with the FontRunItemizer which produced the view.
    // ========================================================================

    struct FontRunView
    {
        const UnicodeScalar* scalars{ nullptr };
        uint32_t scalarCount{ 0 };

        const ShapingCluster* clusters{ nullptr };
        uint32_t clusterCount{ 0 };

        FontFace face{};

        UnicodeScriptIndex script{ kUnicodeScriptIndexInvalid };
        uint8_t bidiLevel{ 0 };

        ScalarIndex normalizedBegin{ 0 };
        SourceRange source{};

        bool completeCoverage{ true };


        [[nodiscard]] bool empty() const noexcept {
            return scalarCount == 0;
        }

        explicit operator bool() const noexcept {
            return !empty();
        }

        [[nodiscard]] bool rightToLeft() const noexcept {
            return (bidiLevel & 1u) != 0;
        }

        [[nodiscard]] bool leftToRight() const noexcept {
            return !rightToLeft();
        }

        [[nodiscard]] const UnicodeScalar* begin() const noexcept {
            return scalars;
        }

        [[nodiscard]] const UnicodeScalar* end() const noexcept
        {
            return scalarCount != 0
                ? scalars + scalarCount
                : scalars;
        }

        [[nodiscard]] const ShapingCluster* clusterBegin() const noexcept {
            return clusters;
        }

        [[nodiscard]] const ShapingCluster* clusterEnd() const noexcept
        {
            return clusterCount != 0
                ? clusters + clusterCount
                : clusters;
        }
    };

} // namespace waavs