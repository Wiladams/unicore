// opentype_sequence_lookup.h
#pragma once

#include <cstdint>

namespace waavs
{
    // ====================================================================
    // OpenTypeSequenceLookup
    //
    // SequenceLookupRecord in the OpenType specification.
    //
    // sequenceIndex:
    //   Position in the matched input sequence.
    //
    // lookupListIndex:
    //   LookupList index of the nested lookup to apply.
    // ====================================================================

    struct OpenTypeSequenceLookup
    {
        uint16_t sequenceIndex{ 0 };
        uint16_t lookupListIndex{ 0 };
    };
}