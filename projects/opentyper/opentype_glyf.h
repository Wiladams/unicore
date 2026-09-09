// opentype_glyf.h
#pragma once

// opentype_glyf.h
#pragma once

#include "opentype_bytestream.h"
#include "pathprogram_builder.h"

#include <array>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>




namespace waavs
{
    namespace opentype
    {
        // ====================================================================
        // TrueType glyf flags
        // ====================================================================

        namespace GlyfSimpleFlags
        {
            static constexpr uint8_t OnCurvePoint = 0x01;
            static constexpr uint8_t XShortVector = 0x02;
            static constexpr uint8_t YShortVector = 0x04;
            static constexpr uint8_t RepeatFlag = 0x08;
            static constexpr uint8_t XSameOrPositive = 0x10;
            static constexpr uint8_t YSameOrPositive = 0x20;
            static constexpr uint8_t OverlapSimple = 0x40;
        }


        namespace GlyfCompositeFlags
        {
            static constexpr uint16_t ArgsAreWords = 0x0001;
            static constexpr uint16_t ArgsAreXYValues = 0x0002;
            static constexpr uint16_t RoundXYToGrid = 0x0004;
            static constexpr uint16_t WeHaveScale = 0x0008;
            static constexpr uint16_t MoreComponents = 0x0020;
            static constexpr uint16_t WeHaveXYScale = 0x0040;
            static constexpr uint16_t WeHaveTwoByTwo = 0x0080;
            static constexpr uint16_t WeHaveInstructions = 0x0100;
            static constexpr uint16_t UseMyMetrics = 0x0200;
            static constexpr uint16_t OverlapCompound = 0x0400;
            static constexpr uint16_t ScaledComponentOffset = 0x0800;
            static constexpr uint16_t UnscaledComponentOffset = 0x1000;
        }


        // ====================================================================
        // Decoded outline representation
        //
        // This is intentionally transient. The compressed TrueType point
        // representation is expanded here, composite glyphs are flattened,
        // then the result is immediately converted to PathProgram.
        // ====================================================================

        struct OpenTypeGlyfPoint
        {
            double x{ 0 };
            double y{ 0 };
            bool onCurve{ false };
        };


        struct OpenTypeGlyfOutline
        {
            std::vector<OpenTypeGlyfPoint> points;
            std::vector<uint32_t> contourEnds;

            void clear() noexcept
            {
                points.clear();
                contourEnds.clear();
            }

            bool empty() const noexcept
            {
                return contourEnds.empty();
            }
        };


        // ====================================================================
        // Composite linear transform
        //
        // OpenType defines:
        //
        //   x' = xscale * x + scale10 * y
        //   y' = scale01 * x + yscale * y
        // ====================================================================

        struct OpenTypeGlyfTransform
        {
            double xscale{ 1.0 };
            double scale01{ 0.0 };
            double scale10{ 0.0 };
            double yscale{ 1.0 };

            OpenTypeGlyfPoint apply(const OpenTypeGlyfPoint& point) const noexcept
            {
                OpenTypeGlyfPoint result;

                result.x = xscale * point.x + scale10 * point.y;
                result.y = scale01 * point.x + yscale * point.y;
                result.onCurve = point.onCurve;

                return result;
            }

            void applyVector(double& x, double& y) const noexcept
            {
                const double tx = xscale * x + scale10 * y;
                const double ty = scale01 * x + yscale * y;

                x = tx;
                y = ty;
            }
        };


        // ====================================================================
        // OpenTypeGlyfDecoder
        //
        // Decodes TrueType glyf/loca data into PathProgram.
        //
        // Coordinates remain in font design units.
        //
        // Supported:
        //
        //   - short loca
        //   - long loca
        //   - simple glyphs
        //   - repeated flags
        //   - compressed x/y deltas
        //   - implicit quadratic on-curve points
        //   - composite glyphs
        //   - nested composites
        //   - component translation
        //   - uniform scale
        //   - independent x/y scale
        //   - 2x2 transforms
        //   - ordinary point-to-point component attachment
        //
        // Deliberately not performed here:
        //
        //   - TrueType hint execution
        //   - gvar variation deltas
        //   - phantom-point component attachment
        //
        // Hint instructions are validated and skipped.
        // ====================================================================

        class OpenTypeGlyfDecoder
        {
        private:
            static constexpr uint32_t kMaxCompositeDepth = 32;

            ByteSpan fGlyf{};
            ByteSpan fLoca{};
            uint32_t fGlyphCount{ 0 };
            int16_t fLocaFormat{ 0 };


        public:
            OpenTypeGlyfDecoder() noexcept = default;

            OpenTypeGlyfDecoder(ByteSpan glyf, ByteSpan loca, uint32_t glyphCount, int16_t locaFormat) noexcept
                : fGlyf(glyf)
                , fLoca(loca)
                , fGlyphCount(glyphCount)
                , fLocaFormat(locaFormat)
            {}


            // ====================================================================
            // Validation
            // ====================================================================

            [[nodiscard]]
            bool isValid() const noexcept
            {
                if (fGlyphCount == 0 || fGlyphCount > 0xFFFFu)
                    return false;

                if (fLocaFormat != 0 && fLocaFormat != 1)
                    return false;

                const size_t entrySize = fLocaFormat == 0 ? 2u : 4u;
                const size_t entryCount = size_t(fGlyphCount) + 1u;

                if (entryCount > std::numeric_limits<size_t>::max() / entrySize)
                    return false;

                return fLoca.size() >= entryCount * entrySize;
            }


            explicit operator bool() const noexcept
            {
                return isValid();
            }


            // ====================================================================
            // glyphPath
            //
            // On success, path is replaced with the decoded glyph outline.
            //
            // An empty-outline glyph is a successful decode and produces an
            // empty terminated PathProgram.
            // ====================================================================

            [[nodiscard]]
            bool glyphPath(uint32_t glyphId, PathProgram& path) const noexcept
            {
                path.clear();

                if (!isValid() || glyphId >= fGlyphCount)
                    return false;

                OpenTypeGlyfOutline outline;
                std::array<uint32_t, kMaxCompositeDepth> stack{};

                if (!decodeGlyph(glyphId, outline, 0, stack))
                    return false;

                PathProgramBuilder builder;

                const size_t contourCount = outline.contourEnds.size();
                const size_t pointCount = outline.points.size();

                builder.reserve(
                    pointCount + contourCount * 2u + 1u,
                    pointCount * 4u + contourCount * 2u);

                if (!emitOutline(outline, builder))
                    return false;

                if (!builder.finalize())
                    return false;

                path = std::move(builder.prog);
                return true;
            }


        private:
            // ====================================================================
            // loca
            // ====================================================================

            [[nodiscard]]
            bool readLocaOffset(uint32_t index, uint32_t& result) const noexcept
            {
                result = 0;

                if (index > fGlyphCount)
                    return false;

                OpenTypeByteStream loca(fLoca);

                if (fLocaFormat == 0)
                {
                    const size_t offset = size_t(index) * 2u;
                    auto entry = loca.subStream(offset, 2);

                    if (!entry.isValid())
                        return false;

                    uint16_t value = 0;

                    if (!entry.readUInt16(value))
                        return false;

                    result = uint32_t(value) * 2u;
                    return true;
                }


                const size_t offset = size_t(index) * 4u;
                auto entry = loca.subStream(offset, 4);

                if (!entry.isValid())
                    return false;

                return entry.readUInt32(result);
            }


            [[nodiscard]]
            bool glyphData(uint32_t glyphId, ByteSpan& result) const noexcept
            {
                result = {};

                if (glyphId >= fGlyphCount)
                    return false;

                uint32_t begin = 0;
                uint32_t end = 0;

                if (!readLocaOffset(glyphId, begin) || !readLocaOffset(glyphId + 1u, end))
                    return false;

                if (begin > end)
                    return false;

                if (size_t(end) > fGlyf.size())
                    return false;

                if (begin == end)
                    return true;

                result = fGlyf.subSpan(begin, size_t(end) - begin);
                return result.size() == size_t(end) - begin;
            }


            // ====================================================================
            // decodeGlyph
            // ====================================================================

            [[nodiscard]]
            bool decodeGlyph(uint32_t glyphId, OpenTypeGlyfOutline& result, uint32_t depth,
                std::array<uint32_t, kMaxCompositeDepth>& stack) const noexcept
            {
                result.clear();

                if (glyphId >= fGlyphCount || depth >= kMaxCompositeDepth)
                    return false;

                for (uint32_t i = 0; i < depth; ++i)
                {
                    if (stack[i] == glyphId)
                        return false;
                }

                stack[depth] = glyphId;


                ByteSpan data;

                if (!glyphData(glyphId, data))
                    return false;

                // loca[n] == loca[n + 1] is a legitimate empty glyph.
                if (data.empty())
                    return true;

                if (data.size() < 10)
                    return false;


                OpenTypeByteStream stream(data);

                int16_t contourCount = 0;
                int16_t xMin = 0;
                int16_t yMin = 0;
                int16_t xMax = 0;
                int16_t yMax = 0;

                if (!stream.readInt16(contourCount) ||
                    !stream.readInt16(xMin) ||
                    !stream.readInt16(yMin) ||
                    !stream.readInt16(xMax) ||
                    !stream.readInt16(yMax))
                {
                    return false;
                }

                (void)xMin;
                (void)yMin;
                (void)xMax;
                (void)yMax;


                if (contourCount >= 0)
                {
                    return decodeSimpleGlyph(stream, static_cast<uint16_t>(contourCount), result);
                }

                return decodeCompositeGlyph(stream, result, depth, stack);
            }


            // ====================================================================
            // Simple glyph
            // ====================================================================

            [[nodiscard]]
            bool decodeSimpleGlyph(OpenTypeByteStream stream, uint16_t contourCount,
                OpenTypeGlyfOutline& result) const noexcept
            {
                result.clear();

                // A zero-contour glyph has no outline. It may contain hinting
                // instructions for phantom points, but those do not affect the
                // PathProgram produced here.
                if (contourCount == 0)
                    return true;


                result.contourEnds.reserve(contourCount);

                uint16_t previousEnd = 0;

                for (uint16_t i = 0; i < contourCount; ++i)
                {
                    uint16_t endPoint = 0;

                    if (!stream.readUInt16(endPoint))
                        return false;

                    if (i != 0 && endPoint <= previousEnd)
                        return false;

                    result.contourEnds.push_back(endPoint);
                    previousEnd = endPoint;
                }


                const size_t pointCount = size_t(previousEnd) + 1u;

                if (pointCount == 0)
                    return false;


                // ------------------------------------------------------------
                // Instructions
                // ------------------------------------------------------------

                uint16_t instructionLength = 0;

                if (!stream.readUInt16(instructionLength))
                    return false;

                if (!stream.skip(instructionLength))
                    return false;


                // ------------------------------------------------------------
                // Expand packed flag stream
                // ------------------------------------------------------------

                std::vector<uint8_t> flags;
                flags.reserve(pointCount);

                while (flags.size() < pointCount)
                {
                    uint8_t flag = 0;

                    if (!stream.readUInt8(flag))
                        return false;

                    flags.push_back(flag);

                    if ((flag & GlyfSimpleFlags::RepeatFlag) == 0)
                        continue;

                    uint8_t repeatCount = 0;

                    if (!stream.readUInt8(repeatCount))
                        return false;

                    if (size_t(repeatCount) > pointCount - flags.size())
                        return false;

                    flags.insert(flags.end(), repeatCount, flag);
                }


                if (flags.size() != pointCount)
                    return false;


                // ------------------------------------------------------------
                // X coordinates
                // ------------------------------------------------------------

                std::vector<OpenTypeGlyfPoint> points(pointCount);
                int64_t x = 0;

                for (size_t i = 0; i < pointCount; ++i)
                {
                    const uint8_t flag = flags[i];
                    int32_t delta = 0;

                    if ((flag & GlyfSimpleFlags::XShortVector) != 0)
                    {
                        uint8_t value = 0;

                        if (!stream.readUInt8(value))
                            return false;

                        delta = (flag & GlyfSimpleFlags::XSameOrPositive) != 0
                            ? int32_t(value)
                            : -int32_t(value);
                    }
                    else if ((flag & GlyfSimpleFlags::XSameOrPositive) == 0)
                    {
                        int16_t value = 0;

                        if (!stream.readInt16(value))
                            return false;

                        delta = value;
                    }


                    x += delta;

                    if (x < std::numeric_limits<int32_t>::min() ||
                        x > std::numeric_limits<int32_t>::max())
                    {
                        return false;
                    }

                    points[i].x = static_cast<double>(x);
                    points[i].onCurve = (flag & GlyfSimpleFlags::OnCurvePoint) != 0;
                }


                // ------------------------------------------------------------
                // Y coordinates
                // ------------------------------------------------------------

                int64_t y = 0;

                for (size_t i = 0; i < pointCount; ++i)
                {
                    const uint8_t flag = flags[i];
                    int32_t delta = 0;

                    if ((flag & GlyfSimpleFlags::YShortVector) != 0)
                    {
                        uint8_t value = 0;

                        if (!stream.readUInt8(value))
                            return false;

                        delta = (flag & GlyfSimpleFlags::YSameOrPositive) != 0
                            ? int32_t(value)
                            : -int32_t(value);
                    }
                    else if ((flag & GlyfSimpleFlags::YSameOrPositive) == 0)
                    {
                        int16_t value = 0;

                        if (!stream.readInt16(value))
                            return false;

                        delta = value;
                    }


                    y += delta;

                    if (y < std::numeric_limits<int32_t>::min() ||
                        y > std::numeric_limits<int32_t>::max())
                    {
                        return false;
                    }

                    points[i].y = static_cast<double>(y);
                }


                result.points = std::move(points);

                return validateOutline(result);
            }


            // ====================================================================
            // Composite glyph
            // ====================================================================

            [[nodiscard]]
            bool decodeCompositeGlyph(OpenTypeByteStream stream, OpenTypeGlyfOutline& result,
                uint32_t depth, std::array<uint32_t, kMaxCompositeDepth>& stack) const noexcept
            {
                result.clear();

                bool firstComponent = true;
                bool haveInstructions = false;
                bool moreComponents = false;


                do
                {
                    uint16_t flags = 0;
                    uint16_t componentGlyphId = 0;

                    if (!stream.readUInt16(flags) || !stream.readUInt16(componentGlyphId))
                        return false;

                    if (componentGlyphId >= fGlyphCount)
                        return false;


                    const bool words = (flags & GlyfCompositeFlags::ArgsAreWords) != 0;
                    const bool xyValues = (flags & GlyfCompositeFlags::ArgsAreXYValues) != 0;

                    // The first component cannot be positioned relative to a
                    // parent point because no parent points exist yet.
                    if (firstComponent && !xyValues)
                        return false;


                    int32_t xOffset = 0;
                    int32_t yOffset = 0;

                    uint32_t parentPointIndex = 0;
                    uint32_t childPointIndex = 0;


                    // --------------------------------------------------------
                    // Arguments
                    // --------------------------------------------------------

                    if (words)
                    {
                        if (xyValues)
                        {
                            int16_t arg1 = 0;
                            int16_t arg2 = 0;

                            if (!stream.readInt16(arg1) || !stream.readInt16(arg2))
                                return false;

                            xOffset = arg1;
                            yOffset = arg2;
                        }
                        else
                        {
                            uint16_t arg1 = 0;
                            uint16_t arg2 = 0;

                            if (!stream.readUInt16(arg1) || !stream.readUInt16(arg2))
                                return false;

                            parentPointIndex = arg1;
                            childPointIndex = arg2;
                        }
                    }
                    else
                    {
                        uint8_t arg1 = 0;
                        uint8_t arg2 = 0;

                        if (!stream.readUInt8(arg1) || !stream.readUInt8(arg2))
                            return false;

                        if (xyValues)
                        {
                            xOffset = static_cast<int8_t>(arg1);
                            yOffset = static_cast<int8_t>(arg2);
                        }
                        else
                        {
                            parentPointIndex = arg1;
                            childPointIndex = arg2;
                        }
                    }


                    // --------------------------------------------------------
                    // Component transform
                    // --------------------------------------------------------

                    const uint16_t transformFlags =
                        flags &
                        (GlyfCompositeFlags::WeHaveScale |
                            GlyfCompositeFlags::WeHaveXYScale |
                            GlyfCompositeFlags::WeHaveTwoByTwo);

                    // These three transform forms are mutually exclusive.
                    if (transformFlags != 0 &&
                        (transformFlags & (transformFlags - 1u)) != 0)
                    {
                        return false;
                    }


                    OpenTypeGlyfTransform transform;

                    if ((flags & GlyfCompositeFlags::WeHaveScale) != 0)
                    {
                        double scale = 1.0;

                        if (!stream.readF2Dot14(scale))
                            return false;

                        transform.xscale = scale;
                        transform.yscale = scale;
                    }
                    else if ((flags & GlyfCompositeFlags::WeHaveXYScale) != 0)
                    {
                        if (!stream.readF2Dot14(transform.xscale) ||
                            !stream.readF2Dot14(transform.yscale))
                        {
                            return false;
                        }
                    }
                    else if ((flags & GlyfCompositeFlags::WeHaveTwoByTwo) != 0)
                    {
                        if (!stream.readF2Dot14(transform.xscale) ||
                            !stream.readF2Dot14(transform.scale01) ||
                            !stream.readF2Dot14(transform.scale10) ||
                            !stream.readF2Dot14(transform.yscale))
                        {
                            return false;
                        }
                    }


                    // --------------------------------------------------------
                    // Recursively decode component
                    // --------------------------------------------------------

                    OpenTypeGlyfOutline child;

                    if (!decodeGlyph(componentGlyphId, child, depth + 1u, stack))
                        return false;


                    // Linear transform is applied before component placement.
                    for (OpenTypeGlyfPoint& point : child.points)
                        point = transform.apply(point);


                    double dx = 0;
                    double dy = 0;


                    // --------------------------------------------------------
                    // XY placement
                    // --------------------------------------------------------

                    if (xyValues)
                    {
                        dx = static_cast<double>(xOffset);
                        dy = static_cast<double>(yOffset);

                        const bool scaledOffset =
                            (flags & GlyfCompositeFlags::ScaledComponentOffset) != 0;

                        const bool unscaledOffset =
                            (flags & GlyfCompositeFlags::UnscaledComponentOffset) != 0;

                        // If both are set, OpenType specifies default rasterizer
                        // behavior. Microsoft/Apple default is unscaled.
                        if (scaledOffset && !unscaledOffset)
                            transform.applyVector(dx, dy);

                        // ROUND_XY_TO_GRID belongs to TrueType grid fitting.
                        // This decoder intentionally preserves design-unit
                        // coordinates and does not execute hinting.
                    }


                    // --------------------------------------------------------
                    // Point-to-point placement
                    // --------------------------------------------------------

                    else
                    {
                        // TrueType also permits phantom points here. This first
                        // outline decoder has no metrics/hinting context, so only
                        // real contour points can be aligned.
                        if (parentPointIndex >= result.points.size() ||
                            childPointIndex >= child.points.size())
                        {
                            return false;
                        }

                        const OpenTypeGlyfPoint& parentPoint =
                            result.points[parentPointIndex];

                        const OpenTypeGlyfPoint& childPoint =
                            child.points[childPointIndex];

                        dx = parentPoint.x - childPoint.x;
                        dy = parentPoint.y - childPoint.y;
                    }


                    for (OpenTypeGlyfPoint& point : child.points)
                    {
                        point.x += dx;
                        point.y += dy;
                    }


                    if (!appendOutline(result, child))
                        return false;


                    haveInstructions =
                        haveInstructions ||
                        (flags & GlyfCompositeFlags::WeHaveInstructions) != 0;

                    moreComponents =
                        (flags & GlyfCompositeFlags::MoreComponents) != 0;

                    firstComponent = false;

                } while (moreComponents);


                // ------------------------------------------------------------
                // Composite instructions
                // ------------------------------------------------------------

                if (haveInstructions)
                {
                    uint16_t instructionLength = 0;

                    if (!stream.readUInt16(instructionLength))
                        return false;

                    if (!stream.skip(instructionLength))
                        return false;
                }


                return validateOutline(result);
            }


            // ====================================================================
            // Outline helpers
            // ====================================================================

            [[nodiscard]]
            static bool validateOutline(const OpenTypeGlyfOutline& outline) noexcept
            {
                if (outline.contourEnds.empty())
                    return outline.points.empty();

                if (outline.points.empty())
                    return false;

                uint32_t previous = 0;

                for (size_t i = 0; i < outline.contourEnds.size(); ++i)
                {
                    const uint32_t end = outline.contourEnds[i];

                    if (end >= outline.points.size())
                        return false;

                    if (i != 0 && end <= previous)
                        return false;

                    previous = end;
                }

                return previous + 1u == outline.points.size();
            }


            [[nodiscard]]
            static bool appendOutline(OpenTypeGlyfOutline& destination,
                const OpenTypeGlyfOutline& source) noexcept
            {
                if (source.points.empty())
                    return source.contourEnds.empty();

                if (!validateOutline(source))
                    return false;


                const size_t base = destination.points.size();

                if (base > std::numeric_limits<uint32_t>::max())
                    return false;

                if (source.points.size() >
                    size_t(std::numeric_limits<uint32_t>::max()) - base)
                {
                    return false;
                }


                destination.points.insert(
                    destination.points.end(),
                    source.points.begin(),
                    source.points.end());


                destination.contourEnds.reserve(
                    destination.contourEnds.size() +
                    source.contourEnds.size());


                for (uint32_t end : source.contourEnds)
                {
                    destination.contourEnds.push_back(
                        static_cast<uint32_t>(base) + end);
                }


                return true;
            }


            static OpenTypeGlyfPoint midpoint(const OpenTypeGlyfPoint& a,
                const OpenTypeGlyfPoint& b) noexcept
            {
                OpenTypeGlyfPoint result;

                result.x = (a.x + b.x) * 0.5;
                result.y = (a.y + b.y) * 0.5;
                result.onCurve = true;

                return result;
            }


            // ====================================================================
            // PathProgram emission
            // ====================================================================

            [[nodiscard]]
            static bool emitOutline(const OpenTypeGlyfOutline& outline, PathProgramBuilder& builder) noexcept
            {
                if (outline.points.empty())
                    return outline.contourEnds.empty();

                if (!validateOutline(outline))
                    return false;

                uint32_t contourBegin = 0;

                for (uint32_t contourEnd : outline.contourEnds)
                {
                    if (!emitContour(outline.points.data(), contourBegin, contourEnd, builder))
                        return false;

                    contourBegin = contourEnd + 1u;
                }

                return contourBegin == outline.points.size();
            }


            [[nodiscard]]
            static bool emitContour(const OpenTypeGlyfPoint* points, uint32_t begin, uint32_t end,
                PathProgramBuilder& builder) noexcept
            {
                if (!points || begin > end)
                    return false;

                const uint32_t count = end - begin + 1u;

                if (count == 0)
                    return false;

                const OpenTypeGlyfPoint& first = points[begin];
                const OpenTypeGlyfPoint& last = points[end];

                OpenTypeGlyfPoint start;
                uint32_t index = 0;
                uint32_t remaining = 0;


                // ------------------------------------------------------------
                // Resolve the TrueType logical contour start.
                //
                // First point on-curve:
                //
                //     start = first
                //
                // First off-curve, last on-curve:
                //
                //     start = last
                //
                // First and last both off-curve:
                //
                //     start = midpoint(last, first)
                // ------------------------------------------------------------

                if (first.onCurve)
                {
                    start = first;
                    index = 1u;
                    remaining = count - 1u;
                }
                else if (last.onCurve)
                {
                    start = last;
                    index = 0;
                    remaining = count - 1u;
                }
                else
                {
                    start = midpoint(last, first);
                    index = 0;
                    remaining = count;
                }


                if (!builder.moveTo(
                    static_cast<float>(start.x),
                    static_cast<float>(start.y)))
                {
                    return false;
                }


                // ------------------------------------------------------------
                // Convert TrueType contour points to explicit PathProgram ops.
                //
                // on -> on
                //
                //     lineTo(on)
                //
                // off -> on
                //
                //     quadTo(off, on)
                //
                // off -> off
                //
                //     quadTo(off, midpoint(off, off))
                //
                // The second off-curve point is retained as the next control.
                // ------------------------------------------------------------

                while (remaining != 0)
                {
                    const OpenTypeGlyfPoint& point = points[begin + index];

                    index = (index + 1u) % count;
                    --remaining;


                    if (point.onCurve)
                    {
                        if (!builder.lineTo(
                            static_cast<float>(point.x),
                            static_cast<float>(point.y)))
                        {
                            return false;
                        }

                        continue;
                    }


                    // --------------------------------------------------------
                    // Last logical point is off-curve.
                    //
                    // Its endpoint is the contour start.
                    // --------------------------------------------------------

                    if (remaining == 0)
                    {
                        if (!builder.quadTo(
                            static_cast<float>(point.x),
                            static_cast<float>(point.y),
                            static_cast<float>(start.x),
                            static_cast<float>(start.y)))
                        {
                            return false;
                        }

                        continue;
                    }


                    const OpenTypeGlyfPoint& next = points[begin + index];


                    // --------------------------------------------------------
                    // off -> on
                    // --------------------------------------------------------

                    if (next.onCurve)
                    {
                        if (!builder.quadTo(
                            static_cast<float>(point.x),
                            static_cast<float>(point.y),
                            static_cast<float>(next.x),
                            static_cast<float>(next.y)))
                        {
                            return false;
                        }

                        index = (index + 1u) % count;
                        --remaining;

                        continue;
                    }


                    // --------------------------------------------------------
                    // off -> off
                    //
                    // TrueType implies an on-curve point at the midpoint.
                    //
                    // Do not consume 'next'. It becomes the control point of
                    // the following quadratic segment.
                    // --------------------------------------------------------

                    const OpenTypeGlyfPoint implied = midpoint(point, next);

                    if (!builder.quadTo(
                        static_cast<float>(point.x),
                        static_cast<float>(point.y),
                        static_cast<float>(implied.x),
                        static_cast<float>(implied.y)))
                    {
                        return false;
                    }
                }


                return builder.close();
            }
        };

    } // namespace opentype
} // namespace waavs