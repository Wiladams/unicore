// test_script_shaping_ir_scalar_replace.h
#pragma once

#include "test_core.h"

#include "script_shaping_ir_builder.h"
#include "script_shaping_ir_executor.h"

namespace waavs
{
    static inline bool runScriptShapingIRScalarReplace()
    {
        static constexpr uint32_t saraAm[] =
        {
            0x0E4D,
            0x0E32
        };

        ScriptShapingIRBuilder builder;

        if (!builder.addScalarReplace(0x0E33, saraAm))
            return false;

        ScriptShapingIR ir;

        if (!builder.finalize(ir))
            return false;

        if (ir.instructions.size() != 1)
            return false;

        if (ir.scalarReplacements.size() != 1)
            return false;

        if (ir.scalarReplacementValues.size() != 2)
            return false;

        if (ir.instructions[0].op != ScriptShapingIROp::ScalarReplace)
            return false;

        const ScriptShapingIRScalarReplace* replacement =
            ir.scalarReplace(ir.instructions[0].payloadIndex);

        if (!replacement)
            return false;

        if (replacement->inputValue != 0x0E33)
            return false;

        if (replacement->replacementOffset != 0)
            return false;

        if (replacement->replacementCount != 2)
            return false;

        if (ir.scalarReplacementValue(*replacement, 0) != 0x0E4D)
            return false;

        if (ir.scalarReplacementValue(*replacement, 1) != 0x0E32)
            return false;


        // ------------------------------------------------------------
        // Input:
        //
        //   KO KAI
        //   SARA AM
        //   MAI EK
        //   SARA AM
        //
        // Expected:
        //
        //   KO KAI
        //   NIKHAHIT
        //   SARA AA
        //   MAI EK
        //   NIKHAHIT
        //   SARA AA
        // ------------------------------------------------------------

        UnicodeScalar scalars[4]{};

        scalars[0].value = 0x0E01;
        scalars[1].value = 0x0E33;
        scalars[2].value = 0x0E48;
        scalars[3].value = 0x0E33;

        FontRunView run{};

        run.scalars = scalars;
        run.scalarCount = 4;

        ScriptShapingBuffer buffer;

        if (!buffer.reset(run))
            return false;

        if (!applyScriptShapingIRScalars(ir, buffer))
            return false;

        if (buffer.size() != 6)
            return false;


        // ------------------------------------------------------------
        // Scalar values.
        // ------------------------------------------------------------

        if (buffer[0].value != 0x0E01)
            return false;

        if (buffer[1].value != 0x0E4D)
            return false;

        if (buffer[2].value != 0x0E32)
            return false;

        if (buffer[3].value != 0x0E48)
            return false;

        if (buffer[4].value != 0x0E4D)
            return false;

        if (buffer[5].value != 0x0E32)
            return false;


        // ------------------------------------------------------------
        // Provenance.
        // ------------------------------------------------------------

        if (buffer[0].scalarOffset != 0 || buffer[0].scalarCount != 1)
            return false;

        if (buffer[1].scalarOffset != 1 || buffer[1].scalarCount != 1)
            return false;

        if (buffer[2].scalarOffset != 1 || buffer[2].scalarCount != 1)
            return false;

        if (buffer[3].scalarOffset != 2 || buffer[3].scalarCount != 1)
            return false;

        if (buffer[4].scalarOffset != 3 || buffer[4].scalarCount != 1)
            return false;

        if (buffer[5].scalarOffset != 3 || buffer[5].scalarCount != 1)
            return false;


        // ------------------------------------------------------------
        // Transient flags.
        //
        // Original source items remain unflagged. Items emitted by
        // ScalarReplace are marked as generated.
        // ------------------------------------------------------------

        if (buffer[0].flags != ScriptShapingItemFlagNone)
            return false;

        if ((buffer[1].flags & ScriptShapingItemFlagGenerated) == 0)
            return false;

        if ((buffer[2].flags & ScriptShapingItemFlagGenerated) == 0)
            return false;

        if (buffer[3].flags != ScriptShapingItemFlagNone)
            return false;

        if ((buffer[4].flags & ScriptShapingItemFlagGenerated) == 0)
            return false;

        if ((buffer[5].flags & ScriptShapingItemFlagGenerated) == 0)
            return false;


        // ------------------------------------------------------------
        // No recursive application within one instruction.
        // ------------------------------------------------------------

        if (!applyScriptShapingIRScalars(ir, buffer))
            return false;

        if (buffer.size() != 6)
            return false;


        return true;
    }


    static inline void testScriptShapingIRScalarReplace()
    {
        const bool passed =
            runScriptShapingIRScalarReplace();

        printf(
            "Script shaping IR scalar replace: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs