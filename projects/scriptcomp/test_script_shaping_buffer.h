// test_script_shaping_buffer.h
#pragma once

#include "test_core.h"

#include "script_shaping_buffer.h"

namespace waavs
{
    static inline bool runScriptShapingBuffer()
    {
        UnicodeScalar scalars[4]{};

        scalars[0].value = 'A';
        scalars[1].value = 'B';
        scalars[2].value = 0x0E01;
        scalars[3].value = 0x0E33;

        FontRunView run{};

        run.scalars = scalars;
        run.scalarCount = 4;

        ScriptShapingBuffer buffer;

        if (!buffer.reset(run))
            return false;

        if (buffer.input() != &run)
            return false;

        if (buffer.size() != 4)
            return false;

        if (buffer.empty())
            return false;


        // ------------------------------------------------------------
        // Identity scalar mapping.
        // ------------------------------------------------------------

        if (buffer[0].value != 'A' || buffer[0].scalarOffset != 0 || buffer[0].scalarCount != 1)
            return false;

        if (buffer[1].value != 'B' || buffer[1].scalarOffset != 1 || buffer[1].scalarCount != 1)
            return false;

        if (buffer[2].value != 0x0E01 || buffer[2].scalarOffset != 2 || buffer[2].scalarCount != 1)
            return false;

        if (buffer[3].value != 0x0E33 || buffer[3].scalarOffset != 3 || buffer[3].scalarCount != 1)
            return false;


        // ------------------------------------------------------------
        // Mutable scalar-domain sequence.
        //
        // Simulate the kind of expansion we will later perform through
        // Script IR. Both resulting items retain the source extent of the
        // original fourth scalar.
        // ------------------------------------------------------------

        ScriptShapingItem first{};
        ScriptShapingItem second{};

        first.value = 0x0E4D;
        first.scalarOffset = 3;
        first.scalarCount = 1;

        second.value = 0x0E32;
        second.scalarOffset = 3;
        second.scalarCount = 1;

        buffer.items().erase(buffer.items().begin() + 3);
        buffer.items().push_back(first);
        buffer.items().push_back(second);

        if (buffer.size() != 5)
            return false;

        if (buffer[3].value != 0x0E4D || buffer[3].scalarOffset != 3 || buffer[3].scalarCount != 1)
            return false;

        if (buffer[4].value != 0x0E32 || buffer[4].scalarOffset != 3 || buffer[4].scalarCount != 1)
            return false;


        // ------------------------------------------------------------
        // clear().
        // ------------------------------------------------------------

        buffer.clear();

        if (!buffer.empty())
            return false;

        if (buffer.size() != 0)
            return false;

        if (buffer.input() != nullptr)
            return false;


        // ------------------------------------------------------------
        // Invalid source sequence.
        // ------------------------------------------------------------

        FontRunView invalid{};
        invalid.scalarCount = 1;
        invalid.scalars = nullptr;

        if (buffer.reset(invalid))
            return false;


        return true;
    }


    static inline void testScriptShapingBuffer()
    {
        const bool passed = runScriptShapingBuffer();

        printf(
            "Script shaping buffer: %s\n",
            passed ? "PASS" : "FAIL");
    }

} // namespace waavs