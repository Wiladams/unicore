// ucd_bidi_brackets_parser.h

#pragma once

#include <cstddef>
#include <cstdint>

#include "lang_span.h"

#include "unicode_database_builder.h"
#include "unicode_database_format.h"


namespace waavs
{
    // ========================================================================
    // UCDBidiBracketsParseError
    // ========================================================================

    enum class UCDBidiBracketsParseError : uint8_t
    {
        None = 0,

        EmptyInput,
        BuilderNotEmpty,

        InvalidFieldCount,
        InvalidCodePoint,
        InvalidPairedCodePoint,
        InvalidBracketType,
        UnexpectedNoneType,

        SelfPair,
        NotStrictlyOrdered,
        BuilderRejectedRecord,

        NoRecords,

        MissingReciprocalPair,
        ReciprocalPairMismatch,
        ReciprocalTypeMismatch
    };


    // ========================================================================
    // UCDBidiBracketsParseResult
    // ========================================================================

    struct UCDBidiBracketsParseResult
    {
        UCDBidiBracketsParseError error{ UCDBidiBracketsParseError::None };

        uint32_t lineNumber{ 0 };
        uint32_t errorCodePoint{ 0xFFFFFFFFu };

        uint32_t recordCount{ 0 };
        uint32_t openCount{ 0 };
        uint32_t closeCount{ 0 };


        [[nodiscard]]
        bool success() const noexcept
        {
            return error == UCDBidiBracketsParseError::None;
        }


        explicit operator bool() const noexcept
        {
            return success();
        }
    };


    // ========================================================================
    // ucdBidiBracketsParseErrorString
    // ========================================================================

    static inline const char* ucdBidiBracketsParseErrorString(
        UCDBidiBracketsParseError error) noexcept
    {
        switch (error)
        {
        case UCDBidiBracketsParseError::None:
            return "no error";

        case UCDBidiBracketsParseError::EmptyInput:
            return "BidiBrackets.txt input is empty";

        case UCDBidiBracketsParseError::BuilderNotEmpty:
            return "bidi bracket builder storage is not empty";

        case UCDBidiBracketsParseError::InvalidFieldCount:
            return "invalid BidiBrackets.txt field count";

        case UCDBidiBracketsParseError::InvalidCodePoint:
            return "invalid bidi bracket code point";

        case UCDBidiBracketsParseError::InvalidPairedCodePoint:
            return "invalid paired bidi bracket code point";

        case UCDBidiBracketsParseError::InvalidBracketType:
            return "invalid bidi paired bracket type";

        case UCDBidiBracketsParseError::UnexpectedNoneType:
            return "unexpected None bidi paired bracket record";

        case UCDBidiBracketsParseError::SelfPair:
            return "bidi bracket maps to itself";

        case UCDBidiBracketsParseError::NotStrictlyOrdered:
            return "bidi bracket records are not strictly ordered";

        case UCDBidiBracketsParseError::BuilderRejectedRecord:
            return "UnicodeDatabaseBuilder rejected bidi bracket record";

        case UCDBidiBracketsParseError::NoRecords:
            return "BidiBrackets.txt contains no records";

        case UCDBidiBracketsParseError::MissingReciprocalPair:
            return "bidi bracket reciprocal pair is missing";

        case UCDBidiBracketsParseError::ReciprocalPairMismatch:
            return "bidi bracket reciprocal mapping does not match";

        case UCDBidiBracketsParseError::ReciprocalTypeMismatch:
            return "bidi bracket reciprocal types do not match";
        }

        return "unknown BidiBrackets.txt parse error";
    }


    // ========================================================================
    // UCDBidiBracketsField
    // ========================================================================

    struct UCDBidiBracketsField
    {
        const uint8_t* begin{ nullptr };
        const uint8_t* end{ nullptr };

        [[nodiscard]]
        bool empty() const noexcept
        {
            return begin == end;
        }

        [[nodiscard]]
        size_t size() const noexcept
        {
            return static_cast<size_t>(end - begin);
        }
    };


    // ========================================================================
    // ucdBidiBracketsIsSpace
    // ========================================================================

    [[nodiscard]]
    static inline bool ucdBidiBracketsIsSpace(uint8_t value) noexcept
    {
        return
            value == ' ' ||
            value == '\t';
    }


    // ========================================================================
    // ucdBidiBracketsTrim
    // ========================================================================

    static inline void ucdBidiBracketsTrim(
        UCDBidiBracketsField& field) noexcept
    {
        while (field.begin < field.end &&
            ucdBidiBracketsIsSpace(*field.begin))
        {
            ++field.begin;
        }

        while (field.end > field.begin &&
            ucdBidiBracketsIsSpace(field.end[-1]))
        {
            --field.end;
        }
    }


    // ========================================================================
    // ucdBidiBracketsFind
    // ========================================================================

    [[nodiscard]]
    static inline const uint8_t* ucdBidiBracketsFind(
        const uint8_t* begin, const uint8_t* end, uint8_t value) noexcept
    {
        while (begin < end)
        {
            if (*begin == value)
                return begin;

            ++begin;
        }

        return end;
    }


    // ========================================================================
    // ucdBidiBracketsHexDigit
    // ========================================================================

    [[nodiscard]]
    static inline bool ucdBidiBracketsHexDigit(
        uint8_t ch, uint32_t& outValue) noexcept
    {
        if (ch >= '0' && ch <= '9')
        {
            outValue = ch - '0';
            return true;
        }

        if (ch >= 'A' && ch <= 'F')
        {
            outValue = ch - 'A' + 10u;
            return true;
        }

        if (ch >= 'a' && ch <= 'f')
        {
            outValue = ch - 'a' + 10u;
            return true;
        }

        return false;
    }


    // ========================================================================
    // ucdParseBidiBracketCodePoint
    // ========================================================================

    [[nodiscard]]
    static inline bool ucdParseBidiBracketCodePoint(
        UCDBidiBracketsField field, uint32_t& outCodePoint) noexcept
    {
        ucdBidiBracketsTrim(field);

        if (field.empty())
            return false;

        uint32_t value = 0;

        for (const uint8_t* p = field.begin; p < field.end; ++p)
        {
            uint32_t digit = 0;

            if (!ucdBidiBracketsHexDigit(*p, digit))
                return false;

            if (value > (0xFFFFFFFFu >> 4))
                return false;

            value =
                (value << 4) |
                digit;
        }

        if (value >= kUnicodeLimit)
            return false;

        outCodePoint = value;
        return true;
    }


    // ========================================================================
    // ucdFindBidiBracketRecord
    //
    // The builder maintains records in strictly increasing code-point order,
    // so reciprocal validation can use binary search.
    // ========================================================================

    [[nodiscard]]
    static inline const UnicodeBidiBracketRecord* ucdFindBidiBracketRecord(
        const std::vector<UnicodeBidiBracketRecord>& records,
        uint32_t codePoint) noexcept
    {
        size_t first = 0;
        size_t last = records.size();

        while (first < last)
        {
            const size_t middle =
                first + ((last - first) >> 1);

            const UnicodeBidiBracketRecord& record =
                records[middle];

            if (record.codePoint < codePoint)
            {
                first = middle + 1;
                continue;
            }

            if (record.codePoint > codePoint)
            {
                last = middle;
                continue;
            }

            return &record;
        }

        return nullptr;
    }


    // ========================================================================
    // ucdParseBidiBrackets
    //
    // Parse Unicode BidiBrackets.txt into UnicodeDatabaseBuilder.
    //
    // File format:
    //
    //      codePoint ; pairedCodePoint ; type
    //
    // where type is:
    //
    //      o   Open
    //      c   Close
    //      n   None
    //
    // BidiBrackets.txt contains explicit Open and Close records. None is
    // represented implicitly by absence from the sparse database array.
    //
    // After parsing, validate:
    //
    //      pair(A) == B
    //      pair(B) == A
    //
    // and:
    //
    //      Open  <-> Close
    //
    // ========================================================================

    static inline bool ucdParseBidiBrackets(const ByteSpan& source,
        UnicodeDatabaseBuilder& database,
        UCDBidiBracketsParseResult& outResult)
    {
        outResult = {};

        if (source.empty())
        {
            outResult.error =
                UCDBidiBracketsParseError::EmptyInput;

            return false;
        }

        if (database.bidiBracketCount() != 0)
        {
            outResult.error =
                UCDBidiBracketsParseError::BuilderNotEmpty;

            return false;
        }


        const uint8_t* cursor =
            source.data();

        const uint8_t* sourceEnd =
            source.data() + source.size();

        bool havePrevious = false;
        uint32_t previousCodePoint = 0;


        while (cursor < sourceEnd)
        {
            ++outResult.lineNumber;


            // ---------------------------------------------------------------
            // Extract one physical line.
            // ---------------------------------------------------------------

            const uint8_t* lineBegin =
                cursor;

            while (cursor < sourceEnd &&
                *cursor != '\r' &&
                *cursor != '\n')
            {
                ++cursor;
            }

            const uint8_t* lineEnd =
                cursor;


            if (cursor < sourceEnd && *cursor == '\r')
                ++cursor;

            if (cursor < sourceEnd && *cursor == '\n')
                ++cursor;


            // ---------------------------------------------------------------
            // Remove trailing comment.
            // ---------------------------------------------------------------

            const uint8_t* comment =
                ucdBidiBracketsFind(
                    lineBegin,
                    lineEnd,
                    '#');

            lineEnd = comment;


            UCDBidiBracketsField line{
                lineBegin,
                lineEnd
            };

            ucdBidiBracketsTrim(line);

            if (line.empty())
                continue;


            // ---------------------------------------------------------------
            // Split three semicolon-separated fields.
            // ---------------------------------------------------------------

            const uint8_t* separator1 =
                ucdBidiBracketsFind(
                    line.begin,
                    line.end,
                    ';');

            if (separator1 == line.end)
            {
                outResult.error =
                    UCDBidiBracketsParseError::InvalidFieldCount;

                return false;
            }


            const uint8_t* separator2 =
                ucdBidiBracketsFind(
                    separator1 + 1,
                    line.end,
                    ';');

            if (separator2 == line.end)
            {
                outResult.error =
                    UCDBidiBracketsParseError::InvalidFieldCount;

                return false;
            }


            if (ucdBidiBracketsFind(
                separator2 + 1,
                line.end,
                ';') != line.end)
            {
                outResult.error =
                    UCDBidiBracketsParseError::InvalidFieldCount;

                return false;
            }


            UCDBidiBracketsField codePointField{
                line.begin,
                separator1
            };

            UCDBidiBracketsField pairedCodePointField{
                separator1 + 1,
                separator2
            };

            UCDBidiBracketsField typeField{
                separator2 + 1,
                line.end
            };


            // ---------------------------------------------------------------
            // Code point.
            // ---------------------------------------------------------------

            uint32_t codePoint = 0;

            if (!ucdParseBidiBracketCodePoint(
                codePointField,
                codePoint))
            {
                outResult.error =
                    UCDBidiBracketsParseError::InvalidCodePoint;

                return false;
            }

            outResult.errorCodePoint =
                codePoint;


            // ---------------------------------------------------------------
            // Paired code point.
            // ---------------------------------------------------------------

            uint32_t pairedCodePoint = 0;

            if (!ucdParseBidiBracketCodePoint(
                pairedCodePointField,
                pairedCodePoint))
            {
                outResult.error =
                    UCDBidiBracketsParseError::InvalidPairedCodePoint;

                return false;
            }


            if (codePoint == pairedCodePoint)
            {
                outResult.error =
                    UCDBidiBracketsParseError::SelfPair;

                return false;
            }


            // ---------------------------------------------------------------
            // Bracket type.
            // ---------------------------------------------------------------

            ucdBidiBracketsTrim(typeField);

            if (typeField.size() != 1)
            {
                outResult.error =
                    UCDBidiBracketsParseError::InvalidBracketType;

                return false;
            }


            UnicodeBidiPairedBracketType type =
                UnicodeBidiPairedBracketType::None;

            switch (*typeField.begin)
            {
            case 'o':
                type = UnicodeBidiPairedBracketType::Open;
                break;

            case 'c':
                type = UnicodeBidiPairedBracketType::Close;
                break;

            case 'n':
                outResult.error =
                    UCDBidiBracketsParseError::UnexpectedNoneType;

                return false;

            default:
                outResult.error =
                    UCDBidiBracketsParseError::InvalidBracketType;

                return false;
            }


            // ---------------------------------------------------------------
            // Persistent ordering.
            // ---------------------------------------------------------------

            if (havePrevious &&
                codePoint <= previousCodePoint)
            {
                outResult.error =
                    UCDBidiBracketsParseError::NotStrictlyOrdered;

                return false;
            }


            // ---------------------------------------------------------------
            // Add persistent record.
            // ---------------------------------------------------------------

            if (!database.addBidiBracket(
                codePoint,
                pairedCodePoint,
                type))
            {
                outResult.error =
                    UCDBidiBracketsParseError::BuilderRejectedRecord;

                return false;
            }


            previousCodePoint =
                codePoint;

            havePrevious =
                true;


            ++outResult.recordCount;

            if (type == UnicodeBidiPairedBracketType::Open)
                ++outResult.openCount;
            else
                ++outResult.closeCount;
        }


        if (outResult.recordCount == 0)
        {
            outResult.error =
                UCDBidiBracketsParseError::NoRecords;

            return false;
        }


        // ====================================================================
        // Whole-dataset reciprocal validation
        // ====================================================================

        const std::vector<UnicodeBidiBracketRecord>& records =
            database.bidiBrackets();


        for (const UnicodeBidiBracketRecord& record : records)
        {
            outResult.errorCodePoint =
                record.codePoint;


            const UnicodeBidiBracketRecord* paired =
                ucdFindBidiBracketRecord(
                    records,
                    record.pairedCodePoint);

            if (!paired)
            {
                outResult.error =
                    UCDBidiBracketsParseError::MissingReciprocalPair;

                return false;
            }


            if (paired->pairedCodePoint !=
                record.codePoint)
            {
                outResult.error =
                    UCDBidiBracketsParseError::ReciprocalPairMismatch;

                return false;
            }


            const UnicodeBidiPairedBracketType type =
                static_cast<UnicodeBidiPairedBracketType>(
                    record.type);

            const UnicodeBidiPairedBracketType pairedType =
                static_cast<UnicodeBidiPairedBracketType>(
                    paired->type);


            if (type == UnicodeBidiPairedBracketType::Open)
            {
                if (pairedType !=
                    UnicodeBidiPairedBracketType::Close)
                {
                    outResult.error =
                        UCDBidiBracketsParseError::ReciprocalTypeMismatch;

                    return false;
                }
            }
            else if (type == UnicodeBidiPairedBracketType::Close)
            {
                if (pairedType !=
                    UnicodeBidiPairedBracketType::Open)
                {
                    outResult.error =
                        UCDBidiBracketsParseError::ReciprocalTypeMismatch;

                    return false;
                }
            }
            else
            {
                outResult.error =
                    UCDBidiBracketsParseError::InvalidBracketType;

                return false;
            }
        }


        outResult.error =
            UCDBidiBracketsParseError::None;

        outResult.errorCodePoint =
            0xFFFFFFFFu;

        return true;
    }

} // namespace waavs