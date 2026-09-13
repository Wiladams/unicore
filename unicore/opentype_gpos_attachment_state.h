// opentype_gpos_attachment_state.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <limits>

#include "shaped_glyph_buffer.h"


namespace waavs
{
    enum class OpenTypeGposAttachmentType : uint8_t
    {
        None = 0,
        Cursive,
        Mark
    };


    struct OpenTypeGposAttachment
    {
        OpenTypeGposAttachmentType type{ OpenTypeGposAttachmentType::None };
        size_t parent{ 0 };

        [[nodiscard]] bool attached() const noexcept
        {
            return type != OpenTypeGposAttachmentType::None;
        }

        void clear() noexcept
        {
            type = OpenTypeGposAttachmentType::None;
            parent = 0;
        }
    };


    class OpenTypeGposAttachmentState
    {
    public:
        void clear() noexcept { fAttachments.clear(); }

        void reset(size_t glyphCount)
        {
            fAttachments.assign(glyphCount, {});
        }

        [[nodiscard]] size_t size() const noexcept { return fAttachments.size(); }

        [[nodiscard]] bool matches(size_t glyphCount) const noexcept
        {
            return fAttachments.size() == glyphCount;
        }

        OpenTypeGposAttachment& operator[](size_t index) noexcept
        {
            return fAttachments[index];
        }

        const OpenTypeGposAttachment& operator[](size_t index) const noexcept
        {
            return fAttachments[index];
        }

    private:
        std::vector<OpenTypeGposAttachment> fAttachments{};
    };


    // ====================================================================
    // reverseOpenTypeGposCursiveAttachment
    //
    // If child already participates in a cursive chain, reverse its old
    // attachment path before attaching it to a new parent.
    //
    // For horizontal text the cursive attachment relation lives in offsetY.
    // ====================================================================

    static inline bool reverseOpenTypeGposCursiveAttachment(
        OpenTypeGposAttachmentState& state, ShapedGlyphBuffer& buffer,
        size_t child, size_t newParent)
    {
        if (!state.matches(buffer.size()) ||
            child >= buffer.size() ||
            newParent >= buffer.size())
        {
            return false;
        }

        struct Edge
        {
            size_t child{ 0 };
            size_t parent{ 0 };
            int32_t minorOffset{ 0 };
        };

        std::vector<Edge> edges;
        std::vector<uint8_t> visited(buffer.size(), 0);

        size_t current = child;

        while (current < buffer.size())
        {
            if (visited[current])
                return false;

            visited[current] = 1;

            OpenTypeGposAttachment& attachment = state[current];

            if (attachment.type != OpenTypeGposAttachmentType::Cursive)
                break;

            const size_t parent = attachment.parent;

            if (parent >= buffer.size() || parent == current)
                return false;

            const int32_t minorOffset =
                buffer[current].placement.offsetY;

            attachment.clear();

            if (parent == newParent)
                break;

            edges.push_back({ current, parent, minorOffset });
            current = parent;
        }

        for (const Edge& edge : edges)
        {
            const int64_t reversed =
                -static_cast<int64_t>(edge.minorOffset);

            if (reversed < INT32_MIN || reversed > INT32_MAX)
                return false;

            state[edge.parent].type = OpenTypeGposAttachmentType::Cursive;
            state[edge.parent].parent = edge.child;

            buffer[edge.parent].placement.offsetY =
                static_cast<int32_t>(reversed);
        }

        return true;
    }


    // ====================================================================
    // attachOpenTypeGposCursive
    // ====================================================================

    static inline bool attachOpenTypeGposCursive(
        OpenTypeGposAttachmentState& state, ShapedGlyphBuffer& buffer,
        size_t child, size_t parent, int32_t minorOffset)
    {
        if (!state.matches(buffer.size()) ||
            child >= buffer.size() ||
            parent >= buffer.size() ||
            child == parent)
        {
            return false;
        }

        if (!reverseOpenTypeGposCursiveAttachment(
            state, buffer, child, parent))
        {
            return false;
        }

        state[child].type = OpenTypeGposAttachmentType::Cursive;
        state[child].parent = parent;

        buffer[child].placement.offsetY = minorOffset;


        // Avoid a direct two-node cycle.

        if (state[parent].type == OpenTypeGposAttachmentType::Cursive &&
            state[parent].parent == child)
        {
            state[parent].clear();
            buffer[parent].placement.offsetY = 0;
        }

        return true;
    }


    // ====================================================================
// attachOpenTypeGposMark
//
// Store the local anchor delta. Final pen-position compensation and
// parent placement propagation happen after all GPOS lookups finish.
// ====================================================================

    static inline bool attachOpenTypeGposMark(
        OpenTypeGposAttachmentState& state, ShapedGlyphBuffer& buffer,
        size_t child, size_t parent, int32_t localX, int32_t localY)
    {
        if (!state.matches(buffer.size()) ||
            child >= buffer.size() ||
            parent >= buffer.size() ||
            parent >= child)
        {
            return false;
        }

        state[child].type = OpenTypeGposAttachmentType::Mark;
        state[child].parent = parent;

        // Anchor attachment overrides previous placement adjustments.

        buffer[child].placement.offsetX = localX;
        buffer[child].placement.offsetY = localY;

        return true;
    }


    // ====================================================================
    // resolveOpenTypeGposAttachments
    //
    // Horizontal shaping finalization.
    //
    // Cursive:
    //   inherit only the parent's cross-stream Y placement.
    //
    // Mark:
    //   inherit both parent offsets and compensate for the pen distance
    //   between parent and child.
    //
    // Our horizontal RTL convention keeps advanceX as a positive logical
    // magnitude, so the X pen displacement changes sign with run direction.
    // ====================================================================

    static inline bool resolveOpenTypeGposAttachments(
        ShapedGlyphBuffer& buffer,
        const OpenTypeGposAttachmentState& state,
        bool runRightToLeft)
    {
        if (!state.matches(buffer.size()))
            return false;

        const size_t glyphCount = buffer.size();

        std::vector<int64_t> prefixAdvanceX(glyphCount + 1, 0);
        std::vector<int64_t> prefixAdvanceY(glyphCount + 1, 0);

        for (size_t i = 0; i < glyphCount; ++i)
        {
            prefixAdvanceX[i + 1] =
                prefixAdvanceX[i] + int64_t(buffer[i].placement.advanceX);

            prefixAdvanceY[i + 1] =
                prefixAdvanceY[i] + int64_t(buffer[i].placement.advanceY);
        }

        std::vector<uint8_t> status(glyphCount, 0);
        std::vector<size_t> path;

        const int64_t minValue = std::numeric_limits<int32_t>::min();
        const int64_t maxValue = std::numeric_limits<int32_t>::max();

        for (size_t start = 0; start < glyphCount; ++start)
        {
            if (status[start] == 2)
                continue;

            path.clear();

            size_t current = start;

            for (;;)
            {
                if (current >= glyphCount)
                    return false;

                if (status[current] == 2)
                    break;

                if (status[current] == 1)
                    return false;

                status[current] = 1;
                path.push_back(current);

                const OpenTypeGposAttachment& attachment =
                    state[current];

                if (!attachment.attached())
                    break;

                if (attachment.parent >= glyphCount ||
                    attachment.parent == current)
                {
                    return false;
                }

                if (attachment.type == OpenTypeGposAttachmentType::Mark &&
                    attachment.parent >= current)
                {
                    return false;
                }

                current = attachment.parent;
            }

            for (size_t i = path.size(); i != 0; --i)
            {
                const size_t glyphIndex = path[i - 1];
                const OpenTypeGposAttachment& attachment =
                    state[glyphIndex];

                if (!attachment.attached())
                {
                    status[glyphIndex] = 2;
                    continue;
                }

                const size_t parent = attachment.parent;

                if (attachment.type == OpenTypeGposAttachmentType::Cursive)
                {
                    const int64_t y =
                        int64_t(buffer[glyphIndex].placement.offsetY) +
                        int64_t(buffer[parent].placement.offsetY);

                    if (y < minValue || y > maxValue)
                        return false;

                    buffer[glyphIndex].placement.offsetY =
                        static_cast<int32_t>(y);
                }
                else if (attachment.type == OpenTypeGposAttachmentType::Mark)
                {
                    int64_t x =
                        int64_t(buffer[glyphIndex].placement.offsetX) +
                        int64_t(buffer[parent].placement.offsetX);

                    int64_t y =
                        int64_t(buffer[glyphIndex].placement.offsetY) +
                        int64_t(buffer[parent].placement.offsetY);

                    const int64_t advanceX =
                        prefixAdvanceX[glyphIndex] -
                        prefixAdvanceX[parent];

                    const int64_t advanceY =
                        prefixAdvanceY[glyphIndex] -
                        prefixAdvanceY[parent];

                    if (runRightToLeft)
                        x += advanceX;
                    else
                        x -= advanceX;

                    y -= advanceY;

                    if (x < minValue || x > maxValue ||
                        y < minValue || y > maxValue)
                    {
                        return false;
                    }

                    buffer[glyphIndex].placement.offsetX =
                        static_cast<int32_t>(x);

                    buffer[glyphIndex].placement.offsetY =
                        static_cast<int32_t>(y);
                }
                else
                {
                    return false;
                }

                status[glyphIndex] = 2;
            }
        }

        return true;
    }


    // ====================================================================
    // Compatibility helper retained for the existing Type 3 test.
    // ====================================================================

    static inline bool resolveOpenTypeGposCursiveAttachments(
        ShapedGlyphBuffer& buffer,
        const OpenTypeGposAttachmentState& state)
    {
        for (size_t i = 0; i < state.size(); ++i)
        {
            if (state[i].type == OpenTypeGposAttachmentType::Mark)
                return false;
        }

        // Direction is irrelevant when the graph contains only cursive
        // attachments because horizontal cursive propagation uses only Y.

        return resolveOpenTypeGposAttachments(buffer, state, false);
    }
    /*
    // ====================================================================
    // resolveOpenTypeGposCursiveAttachments
    //
    // Convert local cursive minor-axis offsets into final offsets.
    //
    // Mark attachments will extend this same finalization stage later.
    // ====================================================================

    static inline bool resolveOpenTypeGposCursiveAttachments(
        ShapedGlyphBuffer& buffer,
        const OpenTypeGposAttachmentState& state)
    {
        if (!state.matches(buffer.size()))
            return false;

        const size_t glyphCount = buffer.size();
        std::vector<uint8_t> status(glyphCount, 0);
        std::vector<size_t> path;

        for (size_t start = 0; start < glyphCount; ++start)
        {
            if (status[start] == 2)
                continue;

            path.clear();

            size_t current = start;

            for (;;)
            {
                if (current >= glyphCount)
                    return false;

                if (status[current] == 2)
                    break;

                if (status[current] == 1)
                    return false;

                status[current] = 1;
                path.push_back(current);

                const OpenTypeGposAttachment& attachment =
                    state[current];

                if (!attachment.attached())
                    break;

                if (attachment.type != OpenTypeGposAttachmentType::Cursive ||
                    attachment.parent >= glyphCount ||
                    attachment.parent == current)
                {
                    return false;
                }

                current = attachment.parent;
            }

            for (size_t i = path.size(); i != 0; --i)
            {
                const size_t glyphIndex = path[i - 1];
                const OpenTypeGposAttachment& attachment =
                    state[glyphIndex];

                if (attachment.type == OpenTypeGposAttachmentType::Cursive)
                {
                    const int64_t offset =
                        int64_t(buffer[glyphIndex].placement.offsetY) +
                        int64_t(buffer[attachment.parent].placement.offsetY);

                    if (offset < INT32_MIN || offset > INT32_MAX)
                        return false;

                    buffer[glyphIndex].placement.offsetY =
                        static_cast<int32_t>(offset);
                }

                status[glyphIndex] = 2;
            }
        }

        return true;
    }
    */
} // namespace waavs