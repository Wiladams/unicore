// test_script_shaping_ir_scalar_move_left.h
#pragma once

#include "test_core.h"

#include "script_shaping_ir_builder.h"

namespace waavs
{
    static inline bool runScriptShapingIRScalarMoveLeft()
    {
        ScriptShapingIRBuilder builder;

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

        if (ir.instructions.size() != 1)
            return false;

        if (ir.scalarMoveLeftAcrossRanges.size() != 1)
            return false;

        if (ir.instructions[0].op != ScriptShapingIROp::ScalarMoveLeftAcrossRange)
            return false;

        const ScriptShapingIRScalarMoveLeftAcrossRange* move =
            ir.scalarMoveLeftAcrossRange(ir.instructions[0].payloadIndex);

        if (!move)
            return false;

        if (move->targetValue != 0x0E4D)
            return false;

        if (move->requiredFlags != ScriptShapingItemFlagGenerated)
            return false;

        if (move->acrossFirst != 0x0E48)
            return false;

        if (move->acrossLast != 0x0E4B)
            return false;


        // ------------------------------------------------------------
        // Invalid range must be rejected at builder time.
        // ------------------------------------------------------------

        ScriptShapingIRBuilder invalidBuilder;

        if (invalidBuilder.addScalarMoveLeftAcrossRange(
            0x0E4D,
            ScriptShapingItemFlagGenerated,
            0x0E4B,
            0x0E48))
        {
            return false;
        }

        return true;
    }


    static inline void testScriptShapingIRScalarMoveLeft()
    {
        const bool passed =
            runScriptShapingIRScalarMoveLeft();

        printf(
            "Script shaping IR scalar move left: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs