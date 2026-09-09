// opentype_gpos_context_view.h
#pragma once

#include "opentype_gsub_context_view.h"

namespace waavs
{
    // ====================================================================
    // GPOS LookupType 7 - Context Positioning
    //
    // The OpenType Sequence Context formats are shared verbatim between:
    //
    //   GSUB Type 5 ContextSubst
    //   GPOS Type 7 ContextPos
    //
    // Reuse the already-proven binary views rather than maintaining a
    // second parser for identical structures.
    //
    // Only the meaning of lookupListIndex differs:
    //
    //   GSUB -> index into the GSUB LookupList
    //   GPOS -> index into the GPOS LookupList
    //
    // Matching and execution remain separate GPOS responsibilities.
    // ====================================================================

    using OpenTypeGposContextRuleView = OpenTypeGsubContextRuleView;
    using OpenTypeGposContextRuleSetView = OpenTypeGsubContextRuleSetView;

    using OpenTypeGposContextClassRuleView = OpenTypeGsubContextClassRuleView;
    using OpenTypeGposContextClassSetView = OpenTypeGsubContextClassSetView;

    using OpenTypeGposContextPosView = OpenTypeGsubContextSubstView;

} // namespace waavs