// pathprogram.h
#pragma once


#include <cstdint>
#include <cstddef>
#include <vector>



// Machinery for a PathProgram
// That represents a sequence of path segment operations
namespace waavs
{
    // These ops represent normalized commands.  That means...
    //  - no relative commands exist
    //  - no implicit lineto after moveto
    //  - arcs are already endpoint-form
    //  - smooth curves are expanded

    enum PathOp : uint8_t {
        OP_END = 0,		// Reprsesent the end of the path program (not Z / close)
        OP_MOVETO,
        OP_LINETO,
        OP_CUBICTO,
        OP_QUADTO,
        OP_ARCTO,
        OP_CLOSE
    };

    // Arity table, how many arguments each op takes
    static constexpr uint8_t kPathOpArity[] = {
        0,  // OP_END
        2,  // OP_MOVETO:   x y
        2,  // OP_LINETO:   x y
        6,  // OP_CUBICTO:  x1 y1 x2 y2 x y
        4,  // OP_QUADTO:   x1 y1 x y
        7,  // OP_ARCTO:    rx ry x-axis-rotation large-arc-flag sweep-flag x y
        0   // OP_CLOSE
    };
    
    // Ensure the ops size and arity table size match
    static_assert(OP_CLOSE + 1 == std::size(kPathOpArity), "PathOp arity table size mismatch");


    // The container for a path program
    //	canonical
    //  comparable
    //  cacheable
    // 
    struct PathProgram
    {
        std::vector<uint8_t> ops;
        std::vector<float> args;

        void clear() noexcept
        {
            ops.clear();
            args.clear();
        }
    };
}


