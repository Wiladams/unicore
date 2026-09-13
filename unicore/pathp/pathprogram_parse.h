#pragma once


#include "pathprogram_builder.h"
#include "pathcommand_normalizer.h"
#include "svg_path_reader.h"

namespace waavs
{


    // ------------------------------------------------------------
    // pathProgram_parse()
    // 
    // build a PathProgram from path data, 
    // represented by SVG <path> 'd' attribute.
    // The program is a canonicalized, normalized representation of the path data,
    // so, there are no relative commands, no implicit lineto after moveto, 
    // arcs are in endpoint form, etc.
    // ------------------------------------------------------------
    static bool pathProgram_parse(const MemSpan& input, PathProgram& outProg)
    {
        SVGPathReader reader(input);
        PathProgramBuilder builder;
        PathCommandNormalizer normalizer(builder);

        SVGPathCommand cmd{};
        float args[7]{};
        bool repeated = false;

        for (;;)
        {
            const SVGPathReadResult result = reader.next(cmd, args, repeated);

            if (result == SVGPathReadResult::End)
                break;

            if (result == SVGPathReadResult::Error)
                return false;

            if (!normalizer.consume(cmd, args, repeated))
                return false;
        }

        if (!builder.end())
            return false;

        outProg = std::move(builder.prog);
        return true;
    }

}

