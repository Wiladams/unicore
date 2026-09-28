// opentype_chain_context_match.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "opentype_shaping_buffer.h"
#include "opentype_shaping_ir.h"
#include "opentype_shaping_execution_stats.h"

namespace waavs
{
    enum class OpenTypeShapingIRResult : uint8_t
    {
        Invalid = 0,
        NoMatch,
        Match
    };

    enum class OpenTypeShapingIRGlyphSearchResult : uint8_t
    {
        Invalid = 0,
        End,
        Found
    };

    struct OpenTypeGsubIRChainContextMatch
    {
        std::vector<size_t> inputPositions{};
        uint32_t lookupOffset{0};
        uint32_t lookupCount{0};

        void clear() noexcept
        {
            inputPositions.clear();
            lookupOffset = 0;
            lookupCount = 0;
        }

        [[nodiscard]] bool empty() const noexcept { return inputPositions.empty(); }
        [[nodiscard]] size_t size() const noexcept { return inputPositions.size(); }
    };

    static inline OpenTypeShapingIRGlyphSearchResult openTypeShapingIRLookupNext(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t currentIndex, size_t& result,
        OpenTypeShapingExecutionStats* stats = nullptr) noexcept;

    static inline OpenTypeShapingIRGlyphSearchResult openTypeShapingIRLookupPrevious(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter,
        const OpenTypeShapingBuffer& buffer, size_t currentIndex, size_t& result,
        OpenTypeShapingExecutionStats* stats = nullptr) noexcept;

    static inline OpenTypeShapingIRResult resolveOpenTypeGsubIRChainContextSubtableUnchecked(
        const OpenTypeShapingIR& ir, const OpenTypeShapingIRLookupFilter& filter,
        const OpenTypeShapingIRGsubChainContextSubtable& subtable,
        const OpenTypeShapingBuffer& buffer, size_t glyphIndex,
        OpenTypeGsubIRChainContextMatch& match,
        OpenTypeShapingExecutionStats* stats = nullptr) noexcept;
}
