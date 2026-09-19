// test_script_shaping_ir_scalar_move_left_executor.h
#pragma once

#include "test_core.h"

#include "script_shaping_ir_builder.h"
#include "script_shaping_ir_executor.h"

namespace waavs
{
    static inline bool runScriptShapingIRScalarMoveLeftExecutor()
    {
        static constexpr uint32_t saraAm[] =
        {
            0x0E4D,
            0x0E32
        };

        ScriptShapingIRBuilder builder;

        if (!builder.addScalarReplace(0x0E33, saraAm))
            return false;

        if (!builder.addScalarMoveLeftAcrossRange(
            0x0E4D,
            ScriptShapingItemFlagGenerated,
            0x0E48,
            0x0E4B))
        {
            return false;
        }

        ScriptShapingIR ir;

        if (!builder.finalize(ir))
            return false;

        if (ir.instructions.size() != 2)
            return false;


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
        //   U+0E4D generated
        //   U+0E32 generated
        //
        // ScalarMoveLeftAcrossRange:
        //
        //   U+0E14
        //   U+0E4D generated
        //   U+0E4B
        //   U+0E32 generated
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

        ScriptShapingBuffer buffer;

        if (!buffer.reset(run))
            return false;

        if (!applyScriptShapingIRScalars(ir, buffer))
            return false;

        if (buffer.size() != 4)
            return false;


        // ------------------------------------------------------------
        // Scalar ordering.
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
        // Provenance moves with the generated item.
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
        // Flags move with the generated item.
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
        // A literal U+0E4D must NOT move because it lacks Generated.
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


    static inline void testScriptShapingIRScalarMoveLeftExecutor()
    {
        const bool passed =
            runScriptShapingIRScalarMoveLeftExecutor();

        printf(
            "Script shaping IR scalar move left executor: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs