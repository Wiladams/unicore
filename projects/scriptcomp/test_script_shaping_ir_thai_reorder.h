// test_script_shaping_ir_thai_reorder.h
#pragma once

#include "test_core.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "script_shaping_buffer.h"
#include "script_shaping_ir_executor.h"
#include "script_shaping_policy_compiler.h"
#include "opentype_types.h"

namespace waavs
{
    static inline bool runScriptShapingIRThaiReorder()
    {
        // ------------------------------------------------------------
        // Input:
        //
        //   U+0E14 DO DEK
        //   U+0E4B MAI CHATTAWA
        //   U+0E33 SARA AM
        //
        // ScalarReplace:
        //
        //   U+0E14
        //   U+0E4B
        //   U+0E4D NIKHAHIT
        //   U+0E32 SARA AA
        //
        // ScalarMoveLeftAcrossRange:
        //
        //   U+0E14
        //   U+0E4D NIKHAHIT
        //   U+0E4B MAI CHATTAWA
        //   U+0E32 SARA AA
        // ------------------------------------------------------------

        UnicodeScalar scalars[3]{};

        scalars[0].value = 0x0E14;
        scalars[1].value = 0x0E4B;
        scalars[2].value = 0x0E33;

        FontRunView run{};

        run.scalars = scalars;
        run.scalarCount = 3;
        run.bidiLevel = 0;
        run.completeCoverage = true;


        // ------------------------------------------------------------
        // Compile current Thai Script IR.
        // ------------------------------------------------------------

        ScriptShapingIR ir;

        if (!compileScriptShapingIRForScript(OTAG("thai"), ir))
            return false;

        if (ir.instructions.size() != 5)
            return false;

        if (ir.instructions[0].op != ScriptShapingIROp::ScalarReplace)
            return false;

        if (ir.instructions[1].op != ScriptShapingIROp::ScalarMoveLeftAcrossRange)
            return false;


        // ------------------------------------------------------------
        // Execute scalar-domain shaping.
        // ------------------------------------------------------------

        ScriptShapingBuffer buffer;

        if (!buffer.reset(run))
            return false;

        if (!applyScriptShapingIRScalars(ir, buffer))
            return false;

        if (buffer.size() != 4)
            return false;


        // ------------------------------------------------------------
        // Final scalar ordering.
        // ------------------------------------------------------------

        if (buffer[0].value != 0x0E14)
            return false;

        if (buffer[1].value != 0x0E4D)
            return false;

        if (buffer[2].value != 0x0E4B)
            return false;

        if (buffer[3].value != 0x0E32)
            return false;


        // ------------------------------------------------------------
        // Provenance.
        //
        // The generated NIKHAHIT moves ahead of the tone mark, but still
        // retains the source extent of the original SARA AM.
        // ------------------------------------------------------------

        if (buffer[0].scalarOffset != 0 || buffer[0].scalarCount != 1)
            return false;

        if (buffer[1].scalarOffset != 2 || buffer[1].scalarCount != 1)
            return false;

        if (buffer[2].scalarOffset != 1 || buffer[2].scalarCount != 1)
            return false;

        if (buffer[3].scalarOffset != 2 || buffer[3].scalarCount != 1)
            return false;


        // ------------------------------------------------------------
        // Transient flags.
        // ------------------------------------------------------------

        if (buffer[0].flags != ScriptShapingItemFlagNone)
            return false;

        if ((buffer[1].flags & ScriptShapingItemFlagGenerated) == 0)
            return false;

        if (buffer[2].flags != ScriptShapingItemFlagNone)
            return false;

        if ((buffer[3].flags & ScriptShapingItemFlagGenerated) == 0)
            return false;


        // ------------------------------------------------------------
        // Literal NIKHAHIT must remain untouched.
        //
        // This proves that the move is constrained by the Generated flag.
        // ------------------------------------------------------------

        UnicodeScalar literalScalars[3]{};

        literalScalars[0].value = 0x0E14;
        literalScalars[1].value = 0x0E4B;
        literalScalars[2].value = 0x0E4D;

        FontRunView literalRun{};

        literalRun.scalars = literalScalars;
        literalRun.scalarCount = 3;
        literalRun.bidiLevel = 0;
        literalRun.completeCoverage = true;

        ScriptShapingBuffer literalBuffer;

        if (!literalBuffer.reset(literalRun))
            return false;

        if (!applyScriptShapingIRScalars(ir, literalBuffer))
            return false;

        if (literalBuffer.size() != 3)
            return false;

        if (literalBuffer[0].value != 0x0E14)
            return false;

        if (literalBuffer[1].value != 0x0E4B)
            return false;

        if (literalBuffer[2].value != 0x0E4D)
            return false;

        if (literalBuffer[2].flags != ScriptShapingItemFlagNone)
            return false;

        return true;
    }


    static inline void testScriptShapingIRThaiReorder()
    {
        const bool passed = runScriptShapingIRThaiReorder();

        printf(
            "Script shaping IR Thai reorder: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs