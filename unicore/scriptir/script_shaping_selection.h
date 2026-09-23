// script_shaping_selection.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include <algorithm>
#include <climits>

#include "script_recognition_result.h"
#include "script_shaping_provenance.h"
#include "script_shaping_selection_types.h"

namespace waavs
{
    // One derived semantic selection.
    //
    // sourceSpans remain stable across shaping-buffer mutation. Current buffer
    // indices are resolved on demand through ScriptShapingItem provenance.

    struct ScriptShapingDerivedSelection
    {
        std::vector<ScriptSpan> sourceSpans{};

        void clear()
        {
            sourceSpans.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return sourceSpans.empty(); }
        [[nodiscard]] size_t size() const noexcept { return sourceSpans.size(); }
    };


    // Runtime storage for shaping-derived selections.
    //
    // Selection ids are one-based, matching the other semantic id types.

    struct ScriptShapingSelectionState
    {
        std::vector<ScriptShapingDerivedSelection> selections{};

        void clear()
        {
            selections.clear();
        }

        [[nodiscard]]
        bool reset(size_t count)
        {
            if (count > uint16_t(-1))
                return false;

            selections.clear();
            selections.resize(count);
            return true;
        }

        [[nodiscard]] size_t size() const noexcept { return selections.size(); }

        [[nodiscard]]
        const ScriptShapingDerivedSelection* selection(
            ScriptShapingSelectionId id) const noexcept
        {
            if (id == kScriptShapingSelectionInvalid)
                return nullptr;

            const size_t index = size_t(id - 1);

            return index < selections.size()
                ? &selections[index]
                : nullptr;
        }

        [[nodiscard]]
        ScriptShapingDerivedSelection* selection(
            ScriptShapingSelectionId id) noexcept
        {
            if (id == kScriptShapingSelectionInvalid)
                return nullptr;

            const size_t index = size_t(id - 1);

            return index < selections.size()
                ? &selections[index]
                : nullptr;
        }
    };


    // Transient resolution of any semantic selection against the current
    // ScriptShapingBuffer.
    //
    // itemIndices become stale after structural buffer mutation.

    struct ScriptShapingResolvedSelection
    {
        ScriptShapingSelectionRef selection{};
        std::vector<uint32_t> itemIndices{};

        void clear()
        {
            selection = {};
            itemIndices.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return itemIndices.empty(); }
        [[nodiscard]] size_t size() const noexcept { return itemIndices.size(); }
    };


    [[nodiscard]]
    static inline bool resolveScriptShapingSelection(
        const ScriptShapingSelectionRef& selection,
        const ScriptRecognitionResult& recognition,
        const ScriptRecognitionUnit& unit,
        const ScriptShapingSelectionState& state,
        const ScriptShapingBuffer& buffer,
        ScriptShapingResolvedSelection& result)
    {
        if (!selection.valid())
            return false;

        ScriptShapingResolvedSelection working{};
        working.selection = selection;


        // ------------------------------------------------------------
        // Current recognition unit.
        // ------------------------------------------------------------

        if (selection.kind == ScriptShapingSelectionKind::Unit)
        {
            if (!collectScriptShapingItemIndicesForSourceSpan(
                buffer,
                unit.span,
                working.itemIndices))
            {
                return false;
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

            // Optional absent role -> valid empty selection.
            if (!binding)
            {
                result = std::move(working);
                return true;
            }

            if (!collectScriptShapingItemIndicesForSourceSpan(
                buffer,
                binding->span,
                working.itemIndices))
            {
                return false;
            }

            result = std::move(working);
            return true;
        }


        // ------------------------------------------------------------
        // Shaping-derived selection.
        // ------------------------------------------------------------

        if (selection.kind == ScriptShapingSelectionKind::Derived)
        {
            const ScriptShapingDerivedSelection* derived =
                state.selection(
                    static_cast<ScriptShapingSelectionId>(
                        selection.id));

            if (!derived)
                return false;


            // Scan the shaping buffer once. An item belongs to the selection
            // when its provenance intersects any stored source span.
            //
            // Because each buffer index is considered only once, overlapping
            // source spans cannot produce duplicate indices.

            for (size_t i = 0; i < buffer.size(); ++i)
            {
                bool selected = false;

                for (const ScriptSpan& span : derived->sourceSpans)
                {
                    if (scriptShapingItemIntersectsSourceSpan(
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

                working.itemIndices.push_back(
                    static_cast<uint32_t>(i));
            }

            result = std::move(working);
            return true;
        }


        return false;
    }

    [[nodiscard]]
    static constexpr bool scriptSpanLess(
        const ScriptSpan& a,
        const ScriptSpan& b) noexcept
    {
        return a.first < b.first ||
            (a.first == b.first && a.count < b.count);
    }


    [[nodiscard]]
    static inline bool normalizeScriptShapingSourceSpans(
        std::vector<ScriptSpan>& spans)
    {
        spans.erase(
            std::remove_if(
                spans.begin(),
                spans.end(),
                [](const ScriptSpan& span)
                {
                    return span.empty();
                }),
            spans.end());

        if (spans.empty())
            return true;

        std::sort(
            spans.begin(),
            spans.end(),
            scriptSpanLess);

        std::vector<ScriptSpan> working;
        working.reserve(spans.size());

        for (const ScriptSpan& span : spans)
        {
            if (working.empty())
            {
                working.push_back(span);
                continue;
            }

            ScriptSpan& previous = working.back();

            const uint64_t previousEnd =
                uint64_t(previous.first) + previous.count;

            const uint64_t spanEnd =
                uint64_t(span.first) + span.count;

            // Merge overlapping or immediately adjacent source spans.
            if (uint64_t(span.first) <= previousEnd)
            {
                const uint64_t mergedEnd =
                    spanEnd > previousEnd
                    ? spanEnd
                    : previousEnd;

                if (mergedEnd > uint64_t(UINT32_MAX) + 1)
                    return false;

                previous.count =
                    static_cast<uint32_t>(
                        mergedEnd - previous.first);

                continue;
            }

            working.push_back(span);
        }

        spans.swap(working);
        return true;
    }




    [[nodiscard]]
    static inline bool assignScriptShapingDerivedSelection(
        ScriptShapingSelectionState& state,
        ScriptShapingSelectionId destination,
        const ScriptSpan* spans,
        size_t spanCount)
    {
        ScriptShapingDerivedSelection* selection =
            state.selection(destination);

        if (!selection)
            return false;

        if (spanCount != 0 && !spans)
            return false;

        std::vector<ScriptSpan> normalized;
        normalized.reserve(spanCount);

        for (size_t i = 0; i < spanCount; ++i)
        {
            if (!spans[i].empty())
                normalized.push_back(spans[i]);
        }

        std::sort(
            normalized.begin(),
            normalized.end(),
            [](const ScriptSpan& a, const ScriptSpan& b)
            {
                if (a.first != b.first)
                    return a.first < b.first;

                return a.count < b.count;
            });

        std::vector<ScriptSpan> merged;
        merged.reserve(normalized.size());

        for (const ScriptSpan& span : normalized)
        {
            if (merged.empty())
            {
                merged.push_back(span);
                continue;
            }

            ScriptSpan& previous = merged.back();

            const uint64_t previousEnd =
                uint64_t(previous.first) + uint64_t(previous.count);

            const uint64_t spanEnd =
                uint64_t(span.first) + uint64_t(span.count);

            if (previousEnd < previous.first ||
                spanEnd < span.first)
            {
                return false;
            }

            if (uint64_t(span.first) > previousEnd)
            {
                merged.push_back(span);
                continue;
            }

            const uint64_t newEnd =
                spanEnd > previousEnd
                ? spanEnd
                : previousEnd;

            if (newEnd > 0xFFFFFFFFull)
                return false;

            previous.count =
                static_cast<uint32_t>(
                    newEnd - uint64_t(previous.first));
        }

        selection->sourceSpans.swap(merged);
        return true;
    }


    [[nodiscard]]
    static inline bool assignScriptShapingDerivedSelection(
        ScriptShapingSelectionState& state,
        ScriptShapingSelectionId destination,
        const ScriptShapingBuffer& buffer,
        const ScriptShapingResolvedSelection& source)
    {
        std::vector<ScriptSpan> spans;
        spans.reserve(source.itemIndices.size());

        for (uint32_t itemIndex : source.itemIndices)
        {
            if (itemIndex >= buffer.size())
                return false;

            const ScriptSpan span =
                scriptShapingItemSourceSpan(buffer[itemIndex]);

            if (!span.empty())
                spans.push_back(span);
        }

        return assignScriptShapingDerivedSelection(
            state,
            destination,
            spans.data(),
            spans.size());
    }
} // namespace waavs