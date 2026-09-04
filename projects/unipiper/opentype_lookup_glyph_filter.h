// opentype_lookup_glyph_filter.h
#pragma once

#include <cstddef>
#include <cstdint>

#include "opentype_gdef_view.h"
#include "opentype_layout_view.h"
#include "opentype_shaping_buffer.h"

namespace waavs
{
    // ====================================================================
    // LookupFlag bits
    //
    // kOpenTypeLookupFlagUseMarkFilteringSet already exists in
    // opentype_layout_view.h.
    // ====================================================================

    static constexpr uint16_t kOpenTypeLookupFlagRightToLeft = 0x0001u;
    static constexpr uint16_t kOpenTypeLookupFlagIgnoreBaseGlyphs = 0x0002u;
    static constexpr uint16_t kOpenTypeLookupFlagIgnoreLigatures = 0x0004u;
    static constexpr uint16_t kOpenTypeLookupFlagIgnoreMarks = 0x0008u;
    static constexpr uint16_t kOpenTypeLookupFlagReservedMask = 0x00E0u;
    static constexpr uint16_t kOpenTypeLookupFlagMarkAttachmentTypeMask = 0xFF00u;


    // ====================================================================
    // OpenTypeLookupGlyphSearchResult
    //
    // Used when walking through the physical shaping buffer while skipping
    // glyphs ignored by the LookupFlag.
    // ====================================================================

    enum class OpenTypeLookupGlyphSearchResult : uint8_t
    {
        Invalid = 0,
        End,
        Found
    };


    // ====================================================================
    // OpenTypeLookupGlyphFilter
    //
    // Interprets one Lookup's LookupFlag against GDEF.
    //
    // IMPORTANT:
    //
    // LookupFlag filtering does not reject the current glyph against which
    // the first lookup input is tested. It applies when walking to other
    // glyphs in the input, backtrack, or lookahead sequence.
    //
    // Therefore this class is primarily used by:
    //
    //   next()
    //   previous()
    //
    // rather than to decide whether a lookup may begin at a buffer position.
    // ====================================================================

    class OpenTypeLookupGlyphFilter
    {
    public:
        OpenTypeLookupGlyphFilter() noexcept = default;

        OpenTypeLookupGlyphFilter(
            const OpenTypeLayoutLookupView& lookup,
            const OpenTypeGdefView& gdef) noexcept
        {
            reset(lookup, gdef);
        }


        // ================================================================
        // reset
        //
        // Validate the LookupFlag and the GDEF structures required by it.
        //
        // Child Coverage tables inside MarkGlyphSetsDef remain lazy.
        // ================================================================

        bool reset(const OpenTypeLayoutLookupView& lookup,
            const OpenTypeGdefView& gdef) noexcept
        {
            clear();

            if (!lookup)
                return false;

            fLookupFlag = lookup.lookupFlag();


            // Reserved LookupFlag bits must be zero.

            if ((fLookupFlag & kOpenTypeLookupFlagReservedMask) != 0)
                return false;


            const bool ignoreBase =
                (fLookupFlag & kOpenTypeLookupFlagIgnoreBaseGlyphs) != 0;

            const bool ignoreLigatures =
                (fLookupFlag & kOpenTypeLookupFlagIgnoreLigatures) != 0;

            const bool ignoreMarks =
                (fLookupFlag & kOpenTypeLookupFlagIgnoreMarks) != 0;

            const bool useMarkFilteringSet =
                (fLookupFlag & kOpenTypeLookupFlagUseMarkFilteringSet) != 0;

            const uint16_t markAttachmentType =
                static_cast<uint16_t>(
                    (fLookupFlag & kOpenTypeLookupFlagMarkAttachmentTypeMask) >> 8);


            const bool needsGlyphClass =
                ignoreBase ||
                ignoreLigatures ||
                ignoreMarks ||
                useMarkFilteringSet ||
                markAttachmentType != 0;


            // ------------------------------------------------------------
            // No filtering information is required.
            //
            // RightToLeft does not affect GSUB glyph filtering.
            // ------------------------------------------------------------

            if (!needsGlyphClass)
            {
                fValid = true;
                return true;
            }


            if (!gdef)
                return false;


            // ------------------------------------------------------------
            // We need GlyphClassDef to distinguish base, ligature, mark,
            // component, and unclassified glyphs.
            // ------------------------------------------------------------

            const OpenTypeClassDefView glyphClasses = gdef.glyphClassDef();

            if (!glyphClasses)
                return false;


            fGdef = gdef;


            // ------------------------------------------------------------
            // Mark filtering set.
            //
            // The MarkFilteringSet field exists in the Lookup only when
            // UseMarkFilteringSet is set.
            // ------------------------------------------------------------

            if (useMarkFilteringSet)
            {
                if (!lookup.markFilteringSet(fMarkFilteringSet))
                    return false;

                const OpenTypeMarkGlyphSetsView sets =
                    gdef.markGlyphSetsDef();

                if (!sets)
                    return false;

                if (fMarkFilteringSet >= sets.size())
                    return false;
            }


            // ------------------------------------------------------------
            // Mark attachment class.
            //
            // IgnoreMarks supersedes both MarkFilteringSet and
            // MarkAttachmentType.
            //
            // MarkFilteringSet supersedes MarkAttachmentType.
            //
            // Therefore MarkAttachClassDef is only required when the
            // attachment type will actually be consulted.
            // ------------------------------------------------------------

            if (!ignoreMarks &&
                !useMarkFilteringSet &&
                markAttachmentType != 0)
            {
                const OpenTypeClassDefView markClasses =
                    gdef.markAttachClassDef();

                if (!markClasses)
                    return false;
            }


            fValid = true;
            return true;
        }


        void clear() noexcept
        {
            fGdef = {};
            fLookupFlag = 0;
            fMarkFilteringSet = 0;
            fValid = false;
        }


        [[nodiscard]] bool isValid() const noexcept { return fValid; }
        explicit operator bool() const noexcept { return isValid(); }

        [[nodiscard]] uint16_t lookupFlag() const noexcept { return fLookupFlag; }

        [[nodiscard]] uint16_t markFilteringSet() const noexcept
        {
            return fMarkFilteringSet;
        }

        [[nodiscard]] uint16_t markAttachmentType() const noexcept
        {
            return static_cast<uint16_t>(
                (fLookupFlag & kOpenTypeLookupFlagMarkAttachmentTypeMask) >> 8);
        }


        // ================================================================
        // shouldSkip
        //
        // Determine whether this glyph should be ignored when matching a
        // non-initial glyph in this lookup.
        //
        // Return value:
        //
        //   true  -> operation succeeded; result contains skip state
        //   false -> malformed or unavailable filtering data
        // ================================================================

        [[nodiscard]] bool shouldSkip(uint32_t glyphId, bool& result) const noexcept
        {
            result = false;

            if (!fValid || glyphId > 0xFFFFu)
                return false;


            const bool ignoreBase =
                (fLookupFlag & kOpenTypeLookupFlagIgnoreBaseGlyphs) != 0;

            const bool ignoreLigatures =
                (fLookupFlag & kOpenTypeLookupFlagIgnoreLigatures) != 0;

            const bool ignoreMarks =
                (fLookupFlag & kOpenTypeLookupFlagIgnoreMarks) != 0;

            const bool useMarkFilteringSet =
                (fLookupFlag & kOpenTypeLookupFlagUseMarkFilteringSet) != 0;

            const uint16_t attachmentType =
                markAttachmentType();


            const bool needsGlyphClass =
                ignoreBase ||
                ignoreLigatures ||
                ignoreMarks ||
                useMarkFilteringSet ||
                attachmentType != 0;


            // No filter bits that depend on glyph class.

            if (!needsGlyphClass)
                return true;


            uint16_t glyphClass = 0;

            if (!fGdef.glyphClass(glyphId, glyphClass))
                return false;


            // ------------------------------------------------------------
            // Base glyph.
            // ------------------------------------------------------------

            if (glyphClass == 1)
            {
                result = ignoreBase;
                return true;
            }


            // ------------------------------------------------------------
            // Ligature glyph.
            // ------------------------------------------------------------

            if (glyphClass == 2)
            {
                result = ignoreLigatures;
                return true;
            }


            // ------------------------------------------------------------
            // Non-mark glyph.
            //
            // Class 0: unclassified
            // Class 4: component
            //
            // Mark filtering rules do not apply to these.
            // ------------------------------------------------------------

            if (glyphClass != 3)
                return true;


            // ------------------------------------------------------------
            // Mark glyph.
            //
            // Precedence:
            //
            //   IgnoreMarks
            //       >
            //   UseMarkFilteringSet
            //       >
            //   MarkAttachmentType
            // ------------------------------------------------------------

            if (ignoreMarks)
            {
                result = true;
                return true;
            }


            if (useMarkFilteringSet)
            {
                const OpenTypeMarkGlyphSetsView sets =
                    fGdef.markGlyphSetsDef();

                if (!sets)
                    return false;

                bool member = false;

                if (!sets.contains(fMarkFilteringSet, glyphId, member))
                    return false;

                result = !member;
                return true;
            }


            if (attachmentType != 0)
            {
                uint16_t markClass = 0;

                if (!fGdef.markAttachClass(glyphId, markClass))
                    return false;

                result = markClass != attachmentType;
                return true;
            }


            return true;
        }


        // ================================================================
        // next
        //
        // Find the next glyph after currentIndex that participates in the
        // lookup after applying LookupFlag filtering.
        //
        // Skipped glyphs remain physically present in the shaping buffer.
        // ================================================================

        [[nodiscard]] OpenTypeLookupGlyphSearchResult next(
            const OpenTypeShapingBuffer& buffer, size_t currentIndex,
            size_t& result) const noexcept
        {
            result = buffer.size();

            if (!fValid || currentIndex >= buffer.size())
                return OpenTypeLookupGlyphSearchResult::Invalid;

            for (size_t index = currentIndex + 1; index < buffer.size(); ++index)
            {
                bool skip = false;

                if (!shouldSkip(buffer[index].glyphId, skip))
                    return OpenTypeLookupGlyphSearchResult::Invalid;

                if (!skip)
                {
                    result = index;
                    return OpenTypeLookupGlyphSearchResult::Found;
                }
            }

            return OpenTypeLookupGlyphSearchResult::End;
        }


        // ================================================================
        // previous
        //
        // Find the previous participating glyph before currentIndex.
        //
        // This will later be used by chaining-context backtrack matching.
        // ================================================================

        [[nodiscard]] OpenTypeLookupGlyphSearchResult previous(
            const OpenTypeShapingBuffer& buffer, size_t currentIndex,
            size_t& result) const noexcept
        {
            result = buffer.size();

            if (!fValid || currentIndex >= buffer.size())
                return OpenTypeLookupGlyphSearchResult::Invalid;

            size_t index = currentIndex;

            while (index != 0)
            {
                --index;

                bool skip = false;

                if (!shouldSkip(buffer[index].glyphId, skip))
                    return OpenTypeLookupGlyphSearchResult::Invalid;

                if (!skip)
                {
                    result = index;
                    return OpenTypeLookupGlyphSearchResult::Found;
                }
            }

            return OpenTypeLookupGlyphSearchResult::End;
        }


    private:
        OpenTypeGdefView fGdef{};
        uint16_t fLookupFlag{ 0 };
        uint16_t fMarkFilteringSet{ 0 };
        bool fValid{ false };
    };

} // namespace waavs