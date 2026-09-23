// script_shaping_provenance.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "script_recognition_types.h"
#include "script_shaping_buffer.h"

namespace waavs
{
    [[nodiscard]]
    static constexpr ScriptSpan scriptShapingItemSourceSpan(
        const ScriptShapingItem& item) noexcept
    {
        return {
            item.scalarOffset,
            item.scalarCount
        };
    }


    [[nodiscard]]
    static constexpr bool scriptSpansIntersect(
        const ScriptSpan& a,
        const ScriptSpan& b) noexcept
    {
        if (a.empty() || b.empty())
            return false;

        const uint64_t aFirst = a.first;
        const uint64_t aEnd = aFirst + a.count;

        const uint64_t bFirst = b.first;
        const uint64_t bEnd = bFirst + b.count;

        return aFirst < bEnd && bFirst < aEnd;
    }


    [[nodiscard]]
    static constexpr bool scriptSpanContains(
        const ScriptSpan& outer,
        const ScriptSpan& inner) noexcept
    {
        if (inner.empty())
            return false;

        const uint64_t outerFirst = outer.first;
        const uint64_t outerEnd = outerFirst + outer.count;

        const uint64_t innerFirst = inner.first;
        const uint64_t innerEnd = innerFirst + inner.count;

        return innerFirst >= outerFirst &&
            innerEnd <= outerEnd;
    }


    [[nodiscard]]
    static constexpr bool scriptShapingItemIntersectsSourceSpan(
        const ScriptShapingItem& item,
        const ScriptSpan& span) noexcept
    {
        return scriptSpansIntersect(
            scriptShapingItemSourceSpan(item),
            span);
    }


    [[nodiscard]]
    static constexpr bool scriptShapingItemWithinSourceSpan(
        const ScriptShapingItem& item,
        const ScriptSpan& span) noexcept
    {
        return scriptSpanContains(
            span,
            scriptShapingItemSourceSpan(item));
    }


    [[nodiscard]]
    static constexpr bool scriptShapingItemContainsSourceSpan(
        const ScriptShapingItem& item,
        const ScriptSpan& span) noexcept
    {
        return scriptSpanContains(
            scriptShapingItemSourceSpan(item),
            span);
    }


    [[nodiscard]]
    static inline size_t countScriptShapingItemsForSourceSpan(
        const ScriptShapingBuffer& buffer,
        const ScriptSpan& span) noexcept
    {
        size_t count = 0;

        for (const ScriptShapingItem& item : buffer)
        {
            if (scriptShapingItemIntersectsSourceSpan(item, span))
                ++count;
        }

        return count;
    }


    [[nodiscard]]
    static inline bool collectScriptShapingItemIndicesForSourceSpan(
        const ScriptShapingBuffer& buffer,
        const ScriptSpan& span,
        std::vector<uint32_t>& result)
    {
        std::vector<uint32_t> working;

        if (buffer.size() > std::numeric_limits<uint32_t>::max())
            return false;

        working.reserve(
            countScriptShapingItemsForSourceSpan(
                buffer,
                span));

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            if (!scriptShapingItemIntersectsSourceSpan(
                buffer[i],
                span))
            {
                continue;
            }

            working.push_back(
                static_cast<uint32_t>(i));
        }

        result = std::move(working);
        return true;
    }

} // namespace waavs
