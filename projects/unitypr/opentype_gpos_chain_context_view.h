// opentype_gpos_chain_context_view.h
#pragma once

#include "opentype_gsub_chain_context_view.h"

namespace waavs
{
    // ====================================================================
    // GPOS LookupType 8 - Chained Context Positioning
    //
    // The OpenType chained sequence-context binary formats are shared by:
    //
    //   GSUB Type 6 ChainContextSubst
    //   GPOS Type 8 ChainContextPos
    //
    // Reuse the proven binary views. Matching and execution semantics remain
    // separate GPOS responsibilities.
    //
    // SequenceLookup.lookupListIndex therefore refers to the GPOS LookupList
    // when these views are used by GPOS.
    // ====================================================================

    using OpenTypeGposChainContextRuleView = OpenTypeGsubChainContextRuleView;
    using OpenTypeGposChainContextRuleSetView = OpenTypeGsubChainContextRuleSetView;

    using OpenTypeGposChainContextClassRuleView = OpenTypeGsubChainContextClassRuleView;
    using OpenTypeGposChainContextClassSetView = OpenTypeGsubChainContextClassSetView;

    using OpenTypeGposChainContextPosView = OpenTypeGsubChainContextSubstView;

} // namespace waavs