// svg_path_command.h

#pragma once

#include <cstdint>

namespace waavs
{
    // SVGPathCommand
    // Represents the individual commands in an SVG path
    enum class SVGPathCommand : uint8_t
    {
        // Move to
        M = 'M',  // absolute moveto
        m = 'm',  // relative moveto

        // Line to
        L = 'L',  // absolute lineto
        l = 'l',  // relative lineto
        H = 'H',  // absolute horizontal lineto
        h = 'h',  // relative horizontal lineto
        V = 'V',  // absolute vertical lineto
        v = 'v',  // relative vertical lineto

        // Cubic Bezier
        C = 'C',  // absolute cubic Bezier
        c = 'c',  // relative cubic Bezier
        S = 'S',  // absolute smooth cubic Bezier
        s = 's',  // relative smooth cubic Bezier

        // Quadratic Bezier
        Q = 'Q',  // absolute quadratic Bezier
        q = 'q',  // relative quadratic Bezier
        T = 'T',  // absolute smooth quadratic Bezier
        t = 't',  // relative smooth quadratic Bezier

        // Elliptical arc
        A = 'A',  // absolute arc
        a = 'a',  // relative arc

        // Close path
        Z = 'Z',  // absolute closepath
        z = 'z'   // relative closepath (treated the same as Z in most renderers)
    };


}