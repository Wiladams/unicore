// opentype_shaping_execution_stats.h
#pragma once

#include <cstdint>

namespace waavs
{
    struct OpenTypeShapingExecutionStats
    {
        uint64_t glyphVisits{0};
        uint64_t lookupAttempts{0};
        uint64_t lookupMatches{0};
        uint64_t subtableAttempts{0};

        uint64_t glyphSetTests{0};
        uint64_t glyphRangeTests{0};

        uint64_t filterTests{0};
        uint64_t filterSkips{0};

        uint64_t contextRuleAttempts{0};
        uint64_t contextPositionTests{0};
        uint64_t contextMatches{0};
        uint64_t nestedLookupCalls{0};

        uint64_t ligatureCandidates{0};
        uint64_t ligatureComponentTests{0};

        uint64_t bufferInsertions{0};
        uint64_t bufferErasures{0};
        uint64_t glyphsMoved{0};

        uint64_t attachmentEdges{0};

        uint64_t chainExecLookups{0};
        uint64_t chainExecFallbackLookups{0};
        uint64_t chainExecStatesVisited{0};
        uint64_t chainExecTransitionsTested{0};
        uint64_t chainExecAcceptsVisited{0};
        uint64_t chainExecCandidatesCollected{0};
        uint64_t chainExecCandidatesTested{0};
        uint64_t chainExecCompiledRuleTests{0};
        uint64_t chainExecBacktrackPositionTests{0};
        uint64_t chainExecLookaheadPositionTests{0};
        uint64_t chainExecProvenBacktrackSkipped{0};
        uint64_t chainExecProvenLookaheadSkipped{0};
        uint64_t chainExecSemanticFallbackCalls{0};

        void clear() noexcept { *this = {}; }

        OpenTypeShapingExecutionStats& operator+=(const OpenTypeShapingExecutionStats& other) noexcept
        {
            glyphVisits += other.glyphVisits;
            lookupAttempts += other.lookupAttempts;
            lookupMatches += other.lookupMatches;
            subtableAttempts += other.subtableAttempts;
            glyphSetTests += other.glyphSetTests;
            glyphRangeTests += other.glyphRangeTests;
            filterTests += other.filterTests;
            filterSkips += other.filterSkips;
            contextRuleAttempts += other.contextRuleAttempts;
            contextPositionTests += other.contextPositionTests;
            contextMatches += other.contextMatches;
            nestedLookupCalls += other.nestedLookupCalls;
            ligatureCandidates += other.ligatureCandidates;
            ligatureComponentTests += other.ligatureComponentTests;
            bufferInsertions += other.bufferInsertions;
            bufferErasures += other.bufferErasures;
            glyphsMoved += other.glyphsMoved;
            attachmentEdges += other.attachmentEdges;
            chainExecLookups += other.chainExecLookups;
            chainExecFallbackLookups += other.chainExecFallbackLookups;
            chainExecStatesVisited += other.chainExecStatesVisited;
            chainExecTransitionsTested += other.chainExecTransitionsTested;
            chainExecAcceptsVisited += other.chainExecAcceptsVisited;
            chainExecCandidatesCollected += other.chainExecCandidatesCollected;
            chainExecCandidatesTested += other.chainExecCandidatesTested;
            chainExecCompiledRuleTests += other.chainExecCompiledRuleTests;
            chainExecBacktrackPositionTests += other.chainExecBacktrackPositionTests;
            chainExecLookaheadPositionTests += other.chainExecLookaheadPositionTests;
            chainExecProvenBacktrackSkipped += other.chainExecProvenBacktrackSkipped;
            chainExecProvenLookaheadSkipped += other.chainExecProvenLookaheadSkipped;
            chainExecSemanticFallbackCalls += other.chainExecSemanticFallbackCalls;
            return *this;
        }
    };
}
