// svg_path_reader.h

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <charconv>
#include <system_error>

#include "svg_path_command.h"
#include "mem_span.h"

namespace waavs
{
    enum class SVGPathReadResult : uint8_t
    {
        Item,
        End,
        Error
    };


    // ------------------------------------------------------------
    // SVG command arity
    //
    // 0xff means the byte is not an SVG path command.
    //
    // inline constexpr gives us one logical header-only definition,
    // initialized entirely at compile time.
    // ------------------------------------------------------------

    inline constexpr uint8_t kSVGPathInvalidArity = 0xff;

    inline constexpr std::array<uint8_t, 256> kSVGPathArity = []() constexpr
        {
            std::array<uint8_t, 256> table{};

            for (size_t i = 0; i < table.size(); ++i)
                table[i] = kSVGPathInvalidArity;

            table['M'] = table['m'] = 2;
            table['L'] = table['l'] = 2;
            table['H'] = table['h'] = 1;
            table['V'] = table['v'] = 1;
            table['C'] = table['c'] = 6;
            table['S'] = table['s'] = 4;
            table['Q'] = table['q'] = 4;
            table['T'] = table['t'] = 2;
            table['A'] = table['a'] = 7;
            table['Z'] = table['z'] = 0;

            return table;
        }();


    // ------------------------------------------------------------
    // SVG path lexical helpers
    // ------------------------------------------------------------

    static constexpr bool svgPath_isWsp(uint8_t ch) noexcept
    {
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
    }


    static constexpr bool svgPath_isNumberStart(uint8_t ch) noexcept
    {
        return (ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == '.';
    }


    static constexpr bool svgPath_isArc(SVGPathCommand cmd) noexcept
    {
        return cmd == SVGPathCommand::A || cmd == SVGPathCommand::a;
    }


    static constexpr bool svgPath_isArcFlagArg(SVGPathCommand cmd, uint8_t argIndex) noexcept
    {
        return svgPath_isArc(cmd) && (argIndex == 3 || argIndex == 4);
    }


    // ------------------------------------------------------------
    // MemSpan scanning helpers
    // ------------------------------------------------------------

    static inline void svgPath_wsp_skip(MemSpan& s) noexcept
    {
        while (!s.empty() && svgPath_isWsp(*s))
            ++s;
    }


    // Skip optional whitespace, at most one comma, then optional whitespace.
    //
    // Consuming at most one comma is intentional. A second comma remains
    // in the input and causes the subsequent argument read to fail.
    static inline void svgPath_sep_skip(MemSpan& s) noexcept
    {
        svgPath_wsp_skip(s);

        if (!s.empty() && *s == ',')
        {
            ++s;
            svgPath_wsp_skip(s);
        }
    }


    // ------------------------------------------------------------
    // SVG number parsing
    // ------------------------------------------------------------

    static inline bool svgPath_number_read(MemSpan& s, float& out) noexcept
    {
        if (s.empty())
            return false;

        const char* begin = reinterpret_cast<const char*>(s.begin());
        const char* end = reinterpret_cast<const char*>(s.end());
        const char* first = begin;
        const char* check = begin;

        // std::from_chars accepts a leading '-', but not a leading '+'.
        // SVG numbers allow either sign.
        if (*first == '+')
        {
            ++first;
            check = first;

            if (check == end)
                return false;

            // Prevent a second sign after the stripped '+'.
            if (*check == '+' || *check == '-')
                return false;
        }
        else if (*check == '-')
        {
            ++check;

            if (check == end)
                return false;
        }

        // Restrict the beginning to SVG numeric syntax. In particular,
        // do not allow values such as inf or nan.
        if (!((*check >= '0' && *check <= '9') || *check == '.'))
            return false;

        float value = 0.0f;
        const auto res = std::from_chars(first, end, value, std::chars_format::general);

        if (res.ec != std::errc{} || res.ptr == first)
            return false;

        s.advance(size_t(res.ptr - begin));
        out = value;
        return true;
    }


    static inline bool svgPath_arcFlag_read(MemSpan& s, float& out) noexcept
    {
        if (s.empty())
            return false;

        if (*s != '0' && *s != '1')
            return false;

        out = float(*s - '0');
        ++s;
        return true;
    }


    // ------------------------------------------------------------
    // SVGPathReader
    //
    // Pull-oriented reader for raw SVG path command tuples.
    //
    // Each Item returns:
    //
    //   cmd        raw SVG command
    //   args       one complete argument tuple
    //   repeated   true if the command was implicitly repeated
    //
    // Example:
    //
    //   M 10 20 30 40 50 60
    //
    // produces:
    //
    //   M 10 20   repeated=false
    //   M 30 40   repeated=true
    //   M 50 60   repeated=true
    //
    // Conversion of repeated M/m tuples to LINETO belongs to the
    // PathCommandNormalizer, not the reader.
    //
    // Error is terminal. Once Error has been returned, all subsequent
    // calls to next() also return Error until reset() is called.
    // ------------------------------------------------------------

    struct SVGPathReader
    {
        MemSpan remains{};

        SVGPathCommand currentCommand{ SVGPathCommand::M };
        uint8_t currentArgCount{ 0 };

        bool hasCommand{ false };
        bool repeated{ false };
        bool failed{ false };


        explicit SVGPathReader(const MemSpan& input) noexcept
            : remains(input)
        {}


        void reset(const MemSpan& input) noexcept
        {
            remains = input;
            currentCommand = SVGPathCommand::M;
            currentArgCount = 0;
            hasCommand = false;
            repeated = false;
            failed = false;
        }


        bool hasError() const noexcept
        {
            return failed;
        }


        const MemSpan& remaining() const noexcept
        {
            return remains;
        }


        SVGPathReadResult next(SVGPathCommand& cmd, float* args, bool& isRepeated) noexcept
        {
            if (failed)
                return SVGPathReadResult::Error;

            svgPath_wsp_skip(remains);

            if (remains.empty())
                return SVGPathReadResult::End;

            const uint8_t ch = *remains;
            const uint8_t arity = kSVGPathArity[ch];


            // --------------------------------------------------------
            // Explicit command
            // --------------------------------------------------------

            if (arity != kSVGPathInvalidArity)
            {
                currentCommand = static_cast<SVGPathCommand>(ch);
                currentArgCount = arity;
                hasCommand = true;
                repeated = false;

                ++remains;

                // Z/z has no arguments and cannot be implicitly repeated.
                if (currentArgCount == 0)
                {
                    cmd = currentCommand;
                    isRepeated = false;
                    hasCommand = false;
                    return SVGPathReadResult::Item;
                }

                // Whitespace may follow an explicit command, but a comma
                // before its first argument is not valid.
                svgPath_wsp_skip(remains);
            }


            // --------------------------------------------------------
            // Implicit repetition of previous command
            // --------------------------------------------------------

            else
            {
                if (!hasCommand || currentArgCount == 0)
                    return fail_();

                // A repeated tuple may begin directly with a number or
                // with comma-wsp separating it from the previous tuple.
                if (!svgPath_isNumberStart(ch) && ch != ',')
                    return fail_();

                repeated = true;
            }


            if (!args)
                return fail_();


            // --------------------------------------------------------
            // Read one complete argument tuple
            // --------------------------------------------------------

            for (uint8_t i = 0; i < currentArgCount; ++i)
            {
                // The first argument following an explicit command only
                // permits whitespace before it. Subsequent arguments and
                // repeated tuples use comma-wsp rules.
                if (i != 0 || repeated)
                    svgPath_sep_skip(remains);
                else
                    svgPath_wsp_skip(remains);

                bool ok = false;

                if (svgPath_isArcFlagArg(currentCommand, i))
                    ok = svgPath_arcFlag_read(remains, args[i]);
                else
                    ok = svgPath_number_read(remains, args[i]);

                if (!ok)
                    return fail_();
            }


            cmd = currentCommand;
            isRepeated = repeated;

            // A following numeric tuple implicitly repeats this command.
            repeated = true;

            return SVGPathReadResult::Item;
        }


    private:
        SVGPathReadResult fail_() noexcept
        {
            failed = true;
            return SVGPathReadResult::Error;
        }
    };
}