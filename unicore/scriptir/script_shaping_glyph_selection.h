// script_shaping_glyph_selection.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "opentype_shaping_buffer.h"
#include "script_recognition_result.h"
#include "script_shaping_selection.h"

namespace waavs
{
    [[nodiscard]]
    static constexpr ScriptSpan openTypeShapingGlyphSourceSpan(
        const OpenTypeShapingGlyph& glyph) noexcept
    {
        return {
            glyph.scalarOffset,
            glyph.scalarCount
        };
    }


    [[nodiscard]]
    static constexpr bool openTypeShapingGlyphIntersectsSourceSpan(
        const OpenTypeShapingGlyph& glyph,
        const ScriptSpan& span) noexcept
    {
        return scriptSpansIntersect(
            openTypeShapingGlyphSourceSpan(glyph),
            span);
    }


    struct ScriptShapingResolvedGlyphSelection
    {
        ScriptShapingSelectionRef selection{};
        std::vector<uint32_t> glyphIndices{};

        void clear()
        {
            selection = {};
            glyphIndices.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return glyphIndices.empty(); }
        [[nodiscard]] size_t size() const noexcept { return glyphIndices.size(); }
    };


    [[nodiscard]]
    static inline bool resolveScriptShapingGlyphSelection(
        const ScriptShapingSelectionRef& selection,
        const ScriptRecognitionResult& recognition,
        const ScriptRecognitionUnit& unit,
        const ScriptShapingSelectionState& state,
        const OpenTypeShapingBuffer& buffer,
        ScriptShapingResolvedGlyphSelection& result)
    {
        if (!selection.valid())
            return false;

        ScriptShapingResolvedGlyphSelection working{};
        working.selection = selection;


        // ------------------------------------------------------------
        // Current recognition unit.
        // ------------------------------------------------------------

        if (selection.kind == ScriptShapingSelectionKind::Unit)
        {
            for (size_t i = 0; i < buffer.size(); ++i)
            {
                if (!openTypeShapingGlyphIntersectsSourceSpan(buffer[i], unit.span))
                    continue;

                if (i > uint32_t(-1))
                    return false;

                working.glyphIndices.push_back(static_cast<uint32_t>(i));
            }

            result = std::move(working);
            return true;
        }


        // ------------------------------------------------------------
        // Recognition role.
        // ------------------------------------------------------------

        if (selection.kind == ScriptShapingSelectionKind::Role)
        {
            const ScriptRoleBinding* binding =
                recognition.roleFor(
                    unit,
                    static_cast<ScriptRoleId>(selection.id));

            // Missing optional role -> valid empty selection.
            if (!binding)
            {
                result = std::move(working);
                return true;
            }

            for (size_t i = 0; i < buffer.size(); ++i)
            {
                if (!openTypeShapingGlyphIntersectsSourceSpan(
                    buffer[i],
                    binding->span))
                {
                    continue;
                }

                if (i > uint32_t(-1))
                    return false;

                working.glyphIndices.push_back(
                    static_cast<uint32_t>(i));
            }

            result = std::move(working);
            return true;
        }


        // ------------------------------------------------------------
        // Derived selection.
        // ------------------------------------------------------------

        if (selection.kind == ScriptShapingSelectionKind::Derived)
        {
            const ScriptShapingDerivedSelection* derived =
                state.selection(
                    static_cast<ScriptShapingSelectionId>(
                        selection.id));

            if (!derived)
                return false;

            for (size_t i = 0; i < buffer.size(); ++i)
            {
                bool selected = false;

                for (const ScriptSpan& span : derived->sourceSpans)
                {
                    if (openTypeShapingGlyphIntersectsSourceSpan(
                        buffer[i],
                        span))
                    {
                        selected = true;
                        break;
                    }
                }

                if (!selected)
                    continue;

                if (i > uint32_t(-1))
                    return false;

                working.glyphIndices.push_back(
                    static_cast<uint32_t>(i));
            }

            result = std::move(working);
            return true;
        }


        return false;
    }

} // namespace waavs