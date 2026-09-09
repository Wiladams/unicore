// unicode_scalar_stream.h

#pragma once

#include <cstddef>
#include <cstdint>

#include "lang_span.h"
#include "core_utf8.h"


namespace waavs
{
    using TextOffset = uint32_t;
    using ScalarIndex = uint32_t;


    static constexpr TextOffset kTextOffsetInvalid = 0xFFFFFFFFu;


    // ========================================================================
    // TextStreamStatus
    //
    // Ready:
    //      The producer may still emit another item.
    //
    // End:
    //      The producer reached clean end-of-input.
    //
    // InvalidInput:
    //      The producer encountered malformed input and cannot continue.
    //
    // End and InvalidInput are terminal states.
    // ========================================================================

    enum class TextStreamStatus : uint8_t
    {
        Ready = 0,
        End,
        InvalidInput
    };


    // ========================================================================
    // SourceRange
    //
    // Half-open byte range in the original source:
    //
    //      [begin, end)
    //
    // Immediately after UTF-8 decoding this range identifies exactly the bytes
    // which encoded the scalar.
    //
    // Later transformations such as normalization may broaden its meaning to a
    // source envelope. Ranges may then overlap or occur out of source order.
    // ========================================================================

    struct SourceRange
    {
        TextOffset begin{ kTextOffsetInvalid };
        TextOffset end{ kTextOffsetInvalid };


        [[nodiscard]]
        constexpr bool valid() const noexcept {
            return begin != kTextOffsetInvalid &&
                end != kTextOffsetInvalid &&
                begin <= end;
        }


        [[nodiscard]]
        constexpr bool empty() const noexcept {
            return valid() && begin == end;
        }


        [[nodiscard]]
        constexpr TextOffset length() const noexcept {
            return valid() ? end - begin : 0;
        }
    };


    // ========================================================================
    // sourceRangeUnion
    //
    // Return the smallest contiguous source envelope containing both ranges.
    // ========================================================================

    [[nodiscard]]
    static constexpr SourceRange sourceRangeUnion(
        const SourceRange& a, const SourceRange& b) noexcept
    {
        if (!a.valid())
            return b;

        if (!b.valid())
            return a;

        return {
            a.begin < b.begin ? a.begin : b.begin,
            a.end > b.end ? a.end : b.end
        };
    }


    // ========================================================================
    // UnicodeScalar
    //
    // One decoded or transformed Unicode scalar together with its source
    // provenance.
    // ========================================================================

    struct UnicodeScalar
    {
        uint32_t value{ 0 };
        SourceRange source{};
    };


    // ========================================================================
    // Utf8ScalarStream
    //
    // Forward-only UTF-8 scalar producer.
    //
    // Input:
    //
    //      ByteSpan containing UTF-8
    //
    // Output:
    //
    //      UnicodeScalar
    //
    // Producer convention:
    //
    //      bool operator()(UnicodeScalar& out)
    //
    //      true:
    //          One scalar was produced.
    //
    //      false:
    //          No scalar was produced. Call status() to distinguish clean EOF
    //          from malformed UTF-8.
    //
    // Malformed UTF-8 is terminal. No replacement character is emitted.
    //
    // errorOffset():
    //
    //      For a rejected UTF-8 byte:
    //          offset of the byte which caused UTF8_REJECT.
    //
    //      For a truncated sequence at EOF:
    //          offset of the first byte of the incomplete sequence.
    //
    //      Otherwise:
    //          kTextOffsetInvalid.
    //
    // The source ByteSpan backing memory must remain valid for the lifetime of
    // this stream.
    // ========================================================================

    class Utf8ScalarStream
    {
    public:
        Utf8ScalarStream() noexcept = default;


        explicit Utf8ScalarStream(const ByteSpan& source) noexcept {
            reset(source);
        }


        // ====================================================================
        // reset
        // ====================================================================

        void reset(const ByteSpan& source) noexcept
        {
            mBegin = source.begin();
            mCurrent = source.begin();
            mEnd = source.end();

            mStatus = TextStreamStatus::Ready;
            mErrorOffset = kTextOffsetInvalid;

            // UINT32_MAX is reserved as the invalid offset value, so the
            // half-open end position must remain below it.
            if (source.size() >= static_cast<size_t>(kTextOffsetInvalid))
            {
                mCurrent = mEnd;
                mStatus = TextStreamStatus::InvalidInput;
                return;
            }

            if (mCurrent == mEnd)
                mStatus = TextStreamStatus::End;
        }


        // ====================================================================
        // operator()
        //
        // Produce the next Unicode scalar.
        // ====================================================================

        bool operator()(UnicodeScalar& out) noexcept
        {
            if (mStatus != TextStreamStatus::Ready)
                return false;


            if (mCurrent >= mEnd)
            {
                mStatus = TextStreamStatus::End;
                return false;
            }


            const uint8_t* scalarBegin = mCurrent;

            uint32_t state = UTF8_ACCEPT;
            uint32_t codepoint = 0;


            while (mCurrent < mEnd)
            {
                const uint8_t* bytePosition = mCurrent;
                const uint8_t byte = *mCurrent++;

                const uint32_t newState =
                    decode(&state, &codepoint, byte);


                if (newState == UTF8_ACCEPT)
                {
                    out.value = codepoint;

                    out.source.begin =
                        offsetOf(scalarBegin);

                    out.source.end =
                        offsetOf(mCurrent);

                    return true;
                }


                if (newState == UTF8_REJECT)
                {
                    mErrorOffset = offsetOf(bytePosition);
                    mCurrent = mEnd;
                    mStatus = TextStreamStatus::InvalidInput;

                    return false;
                }
            }


            // Input ended while a multibyte scalar was still incomplete.
            mErrorOffset = offsetOf(scalarBegin);
            mStatus = TextStreamStatus::InvalidInput;

            return false;
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]]
        TextStreamStatus status() const noexcept {
            return mStatus;
        }


        [[nodiscard]]
        bool ready() const noexcept {
            return mStatus == TextStreamStatus::Ready;
        }


        [[nodiscard]]
        bool ended() const noexcept {
            return mStatus == TextStreamStatus::End;
        }


        [[nodiscard]]
        bool failed() const noexcept {
            return mStatus == TextStreamStatus::InvalidInput;
        }


        [[nodiscard]]
        TextOffset errorOffset() const noexcept {
            return mErrorOffset;
        }


        // ====================================================================
        // Source position
        //
        // Current byte position in the original UTF-8 source.
        // ====================================================================

        [[nodiscard]]
        TextOffset sourceOffset() const noexcept
        {
            if (!mBegin || !mCurrent)
                return 0;

            return offsetOf(mCurrent);
        }


    private:
        const uint8_t* mBegin{ nullptr };
        const uint8_t* mCurrent{ nullptr };
        const uint8_t* mEnd{ nullptr };

        TextStreamStatus mStatus{ TextStreamStatus::End };
        TextOffset mErrorOffset{ kTextOffsetInvalid };


        [[nodiscard]]
        TextOffset offsetOf(const uint8_t* ptr) const noexcept
        {
            return static_cast<TextOffset>(ptr - mBegin);
        }
    };


    static_assert(sizeof(SourceRange) == 8,
        "SourceRange must be exactly 8 bytes");

    static_assert(sizeof(UnicodeScalar) == 12,
        "UnicodeScalar must be exactly 12 bytes");

} // namespace waavs