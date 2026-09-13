// svg_path_data_sink.h
#pragma once

#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace waavs {

    class SVGPathDataSink
    {
    public:
        std::string data;

        bool onMoveTo(float x, float y)
        {
            if (!command('M') || !number(x) || !number(y))
                return false;
            return true;
        }

        bool onLineTo(float x, float y)
        {
            if (!command('L') || !number(x) || !number(y))
                return false;
            return true;
        }

        bool onQuadTo(float x1, float y1, float x, float y)
        {
            if (!command('Q') || !number(x1) || !number(y1) || !number(x) || !number(y))
                return false;
            return true;
        }

        bool onCubicTo(float x1, float y1, float x2, float y2, float x, float y)
        {
            if (!command('C') || !number(x1) || !number(y1) || !number(x2) || !number(y2) || !number(x) || !number(y))
                return false;
            return true;
        }

        bool onArcTo(float rx, float ry, float rotation, float largeArc, float sweep, float x, float y)
        {
            if (!command('A') || !number(rx) || !number(ry) || !number(rotation))
                return false;
            if (!flag(largeArc) || !flag(sweep) || !number(x) || !number(y))
                return false;
            return true;
        }

        bool onClose()
        {
            return command('Z');
        }

        bool onEnd()
        {
            return true;
        }

        void clear()
        {
            data.clear();
        }

    private:
        bool command(char op)
        {
            if (!data.empty())
                data.push_back(' ');
            data.push_back(op);
            return true;
        }

        bool number(float value)
        {
            if (!std::isfinite(value))
                return false;

            if (value == 0.0f)
                value = 0.0f;

            char buffer[64];
            auto result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general);

            if (result.ec != std::errc())
                return false;

            data.push_back(' ');
            data.append(buffer, result.ptr);
            return true;
        }

        bool flag(float value)
        {
            data.push_back(' ');
            data.push_back(value != 0.0f ? '1' : '0');
            return true;
        }
    };

}