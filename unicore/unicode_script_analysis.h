// unicode_script_analysis.h

#pragma once

#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>

#include "unicode_database.h"
#include "unicode_script.h"
#include "unicode_script_set.h"
#include "unicode_grapheme_stream.h"


namespace waavs
{
    // ========================================================================
    // ScriptGrapheme
    //
    // Script information derived locally from one grapheme cluster.
    //
    // candidates:
    //
    //      Scripts in which this grapheme may participate.
    //
    // script:
    //
    //      Effective Script when local analysis resolves the grapheme to one
    //      Script value.
    //
    //      kUnicodeScriptIndexInvalid means that contextual resolution is
    //      still required.
    //
    // ========================================================================

    struct ScriptGrapheme
    {
        GraphemeClusterView grapheme{};
        UnicodeScriptSet candidates{};
        UnicodeScriptIndex script{ kUnicodeScriptIndexInvalid };

        [[nodiscard]] bool resolved() const noexcept
        {
            return script != kUnicodeScriptIndexInvalid;
        }

        //[[nodiscard]] bool needsContext() const noexcept
        //{
        //    return script == kUnicodeScriptIndexInvalid;
        //}
    };


    // ========================================================================
    // classifyGraphemeScript
    //
    // Perform local Script analysis of one grapheme.
    //
    // No neighboring grapheme context is considered here.
    //
    // ========================================================================

    [[nodiscard]]
    static inline bool classifyGraphemeScript(const GraphemeClusterView& grapheme,
        const UnicodeDatabase& database, ScriptGrapheme& out) noexcept
    {
        out = {};
        out.grapheme = grapheme;

        if (!grapheme)
            return false;

        bool haveCandidates = false;

        for (const UnicodeScalar& scalar : grapheme)
        {
            const UnicodeScriptSet scalarScripts =
                database.scriptExtensions(scalar.value);

            if (scalarScripts.empty())
                continue;

            if (!haveCandidates)
            {
                out.candidates = scalarScripts;
                haveCandidates = true;
                continue;
            }

            UnicodeScriptSet intersection =
                out.candidates.intersection(scalarScripts);

            if (!intersection.empty())
                out.candidates = intersection;
        }

        if (!haveCandidates)
            return true;

        if (out.candidates.isSingleton())
            out.script = out.candidates.first();

        return true;
    }

    // ========================================================================
// isContextualScript
//
// Common and Inherited Script values normally require surrounding context.
// An invalid Script represents a locally ambiguous Script_Extensions set.
// ========================================================================

    [[nodiscard]]
    static inline bool isContextualScript(UnicodeScriptIndex script,
        UnicodeScriptIndex commonScript,
        UnicodeScriptIndex inheritedScript) noexcept
    {
        return script == kUnicodeScriptIndexInvalid ||
            script == commonScript ||
            script == inheritedScript;
    }


    // ========================================================================
    // isStrongScript
    //
    // For this first resolver, a strong Script is simply a locally resolved
    // Script other than Common or Inherited.
    // ========================================================================

    [[nodiscard]]
    static inline bool isStrongScript(UnicodeScriptIndex script,
        UnicodeScriptIndex commonScript,
        UnicodeScriptIndex inheritedScript) noexcept
    {
        return script != kUnicodeScriptIndexInvalid &&
            script != commonScript &&
            script != inheritedScript;
    }


    // ========================================================================
    // canResolveScriptTo
    //
    // Determine whether a context-sensitive grapheme may adopt targetScript.
    //
    // Common and Inherited can adopt surrounding Script context directly.
    //
    // A locally ambiguous Script_Extensions set may adopt targetScript only
    // when targetScript is actually one of its candidates.
    // ========================================================================

    [[nodiscard]]
    static inline bool canResolveScriptTo(const ScriptGrapheme& grapheme,
        UnicodeScriptIndex targetScript,
        UnicodeScriptIndex commonScript,
        UnicodeScriptIndex inheritedScript) noexcept
    {
        if (grapheme.script == commonScript ||
            grapheme.script == inheritedScript)
        {
            return true;
        }

        if (grapheme.script != kUnicodeScriptIndexInvalid)
            return grapheme.script == targetScript;

        return grapheme.candidates.contains(targetScript);
    }


    // ========================================================================
    // resolveMiddleScriptContext
    //
    // Resolve one context-sensitive grapheme using its immediate neighbors.
    //
    // Initial rule:
    //
    //      strong A | contextual | strong A
    //
    // becomes:
    //
    //      strong A | strong A   | strong A
    //
    // No resolution occurs when:
    //
    //      - either neighbor is not strong;
    //      - the neighboring Scripts differ;
    //      - the middle grapheme is already strongly resolved;
    //      - an ambiguous Script_Extensions set does not contain the
    //        surrounding Script.
    //
    // candidates remains unchanged. It records the locally derived Script
    // information; script records the effective contextually resolved Script.
    // ========================================================================

    [[nodiscard]]
    static inline bool resolveMiddleScriptContext(const ScriptGrapheme& left,
        ScriptGrapheme& middle, const ScriptGrapheme& right,
        UnicodeScriptIndex commonScript,
        UnicodeScriptIndex inheritedScript) noexcept
    {
        if (!isStrongScript(left.script, commonScript, inheritedScript) ||
            !isStrongScript(right.script, commonScript, inheritedScript))
        {
            return false;
        }

        if (left.script != right.script)
            return false;

        if (!isContextualScript(
            middle.script,
            commonScript,
            inheritedScript))
        {
            return false;
        }

        if (!canResolveScriptTo(
            middle,
            left.script,
            commonScript,
            inheritedScript))
        {
            return false;
        }

        middle.script = left.script;
        return true;
    }

    struct PendingScriptGrapheme
    {
        std::vector<UnicodeScalar> scalars;

        ScalarIndex normalizedBegin{ 0 };
        SourceRange source{};

        UnicodeScriptSet candidates{};
        UnicodeScriptIndex script{ kUnicodeScriptIndexInvalid };

        [[nodiscard]] ScriptGrapheme view() const noexcept
        {
            ScriptGrapheme result{};

            result.grapheme.scalars =
                scalars.empty() ? nullptr : scalars.data();

            result.grapheme.scalarCount =
                static_cast<uint32_t>(scalars.size());

            result.grapheme.normalizedBegin =
                normalizedBegin;

            result.grapheme.source =
                source;

            result.candidates =
                candidates;

            result.script =
                script;

            return result;
        }
    };


    // ========================================================================
// UnicodeScriptStream
//
// Pull stream:
//
//      GraphemeClusterView
//              |
//              v
//      local Script classification
//              |
//              v
//      contextual Script resolution
//              |
//              v
//          ScriptGrapheme
//
// The pending queue provides look-ahead for context-sensitive graphemes.
//
// The initial implementation uses only the three-grapheme rule already
// proven by resolveMiddleScriptContext(). The queue is deliberately used
// rather than a fixed three-item window so contextual spans can be extended
// later without changing the stream architecture.
// ========================================================================

    template<typename Source>
    class UnicodeScriptStream
    {
    public:
        UnicodeScriptStream(Source& source, const UnicodeDatabase& database) noexcept
            : mSource(&source),
            mDatabase(&database)
        {
            mCommonScript =
                findScript("Zyyy");

            mInheritedScript =
                findScript("Zinh");

            if (mCommonScript == kUnicodeScriptIndexInvalid ||
                mInheritedScript == kUnicodeScriptIndexInvalid)
            {
                mStatus = TextStreamStatus::InvalidInput;
            }
        }


        [[nodiscard]]
        TextStreamStatus status() const noexcept
        {
            return mStatus;
        }


        bool operator()(ScriptGrapheme& out)
        {
            out = {};

            if (mStatus == TextStreamStatus::InvalidInput ||
                mStatus == TextStreamStatus::End)
            {
                return false;
            }

            return pull(out);
        }


    private:
        Source* mSource{ nullptr };
        const UnicodeDatabase* mDatabase{ nullptr };

        UnicodeScriptIndex mCommonScript{ kUnicodeScriptIndexInvalid };
        UnicodeScriptIndex mInheritedScript{ kUnicodeScriptIndexInvalid };

        std::deque<PendingScriptGrapheme> mPending{};
        std::vector<UnicodeScalar> mOutputScalars{};

        TextStreamStatus mStatus{ TextStreamStatus::Ready };
        bool mSourceEnded{ false };


        // ====================================================================
        // findScript
        //
        // Resolve one ISO 15924 Script tag once during stream construction.
        // ====================================================================

        [[nodiscard]]
        UnicodeScriptIndex findScript(const char* iso15924) const noexcept
        {
            if (!mDatabase || !iso15924)
                return kUnicodeScriptIndexInvalid;

            for (uint32_t i = 0; i < mDatabase->scriptCount(); ++i)
            {
                InternedKey tag =
                    mDatabase->scriptISO15924(i);

                if (tag &&
                    std::strcmp(tag, iso15924) == 0)
                {
                    return static_cast<UnicodeScriptIndex>(i);
                }
            }

            return kUnicodeScriptIndexInvalid;
        }


        // ====================================================================
        // readOne
        //
        // Pull and locally classify one grapheme from the upstream source.
        // ====================================================================

        bool readOne()
        {
            if (mSourceEnded)
                return false;

            GraphemeClusterView grapheme{};

            if (!(*mSource)(grapheme))
            {
                mSourceEnded = true;

                if (mSource->status() == TextStreamStatus::InvalidInput)
                    mStatus = TextStreamStatus::InvalidInput;

                return false;
            }


            ScriptGrapheme classified{};

            if (!classifyGraphemeScript(
                grapheme,
                *mDatabase,
                classified))
            {
                mStatus = TextStreamStatus::InvalidInput;
                return false;
            }


            PendingScriptGrapheme pending{};

            pending.scalars.assign( grapheme.begin(), grapheme.end());

            pending.normalizedBegin =  grapheme.normalizedBegin;
            pending.source = grapheme.source;
            pending.candidates = classified.candidates;
            pending.script = classified.script;


            mPending.push_back(std::move(pending));

            return true;
        }


        // ====================================================================
        // fillThree
        //
        // The first implementation needs at most three items to apply the
        // currently proven contextual rule.
        // ====================================================================

        void fillThree()
        {
            while (mPending.size() < 3 &&
                !mSourceEnded &&
                mStatus == TextStreamStatus::Ready)
            {
                if (!readOne())
                    break;
            }
        }

        // ====================================================================
// resolvePendingMiddle
//
// Apply the tested three-grapheme contextual rule to owned pending
// graphemes.
//
// Temporary ScriptGrapheme views are valid for the duration of this
// operation because the owning queue entries are not modified.
// ====================================================================

        void resolvePendingMiddle()
        {
            if (mPending.size() < 3)
                return;

            const ScriptGrapheme left =
                mPending[0].view();

            ScriptGrapheme middle =
                mPending[1].view();

            const ScriptGrapheme right =
                mPending[2].view();

            if (resolveMiddleScriptContext(
                left,
                middle,
                right,
                mCommonScript,
                mInheritedScript))
            {
                mPending[1].script =
                    middle.script;
            }
        }

        // ====================================================================
// emitFront
//
// Move the oldest pending grapheme into output-owned scalar storage and
// return a borrowed ScriptGrapheme view.
//
// The returned view remains valid until the next stream pull.
// ====================================================================

        bool emitFront(ScriptGrapheme& out)
        {
            if (mPending.empty())
                return false;

            PendingScriptGrapheme& pending = mPending.front();

            mOutputScalars = std::move(pending.scalars);

            out = {};

            out.grapheme.scalars =
                mOutputScalars.empty()
                ? nullptr
                : mOutputScalars.data();

            out.grapheme.scalarCount =
                static_cast<uint32_t>(
                    mOutputScalars.size());

            out.grapheme.normalizedBegin =
                pending.normalizedBegin;

            out.grapheme.source =
                pending.source;

            out.candidates =
                pending.candidates;

            out.script =
                pending.script;

            mPending.pop_front();

            return true;
        }

        // ====================================================================
        // pull
        // ====================================================================

        bool pull(ScriptGrapheme& out)
        {
            fillThree();

            if (mStatus == TextStreamStatus::InvalidInput)
                return false;

            if (mPending.empty())
            {
                mStatus = TextStreamStatus::End;
                return false;
            }

            resolvePendingMiddle();

            return emitFront(out);
        }
    };
} // namespace waavs

