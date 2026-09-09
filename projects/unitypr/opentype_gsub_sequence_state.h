// opentype_gsub_sequence_state.h
#pragma once

#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include "opentype_gsub_edit.h"


namespace waavs
{
    // ====================================================================
    // OpenTypeGsubSequenceState
    //
    // Tracks the current physical positions corresponding to the input
    // sequence of a contextual or chaining-contextual substitution.
    //
    // Initial example:
    //
    //   physical buffer:
    //
    //       A mark B mark C
    //       0  1   2  3   4
    //
    //   positions:
    //
    //       { 0, 2, 4 }
    //
    // If B expands to X Y Z:
    //
    //       A mark X Y Z mark C
    //       0  1   2 3 4  5   6
    //
    //   positions:
    //
    //       { 0, 2, 3, 4, 6 }
    //
    // The state has no fixed physical end boundary. Nested contextual
    // lookups are allowed to inspect or consume glyphs beyond the input
    // sequence matched by the outer contextual lookup.
    // ====================================================================

    class OpenTypeGsubSequenceState
    {
    public:
        OpenTypeGsubSequenceState() noexcept = default;

        OpenTypeGsubSequenceState(const size_t* positions, size_t count)
        {
            reset(positions, count);
        }


        // ================================================================
        // reset
        //
        // The pointer/count form deliberately keeps the public abstraction
        // independent of std::vector so the backing storage can be changed
        // later without changing callers.
        // ================================================================

        [[nodiscard]] bool reset(const size_t* positions, size_t count)
        {
            fPositions.clear();

            if (!positions || count == 0)
                return false;

            for (size_t i = 1; i < count; ++i)
            {
                if (positions[i] <= positions[i - 1])
                    return false;
            }

            fPositions.assign(positions, positions + count);
            return true;
        }

        void clear() noexcept
        {
            fPositions.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return fPositions.empty(); }
        [[nodiscard]] size_t size() const noexcept { return fPositions.size(); }
        [[nodiscard]] const size_t* data() const noexcept { return fPositions.data(); }

        [[nodiscard]] bool position(size_t sequenceIndex, size_t& physicalIndex) const noexcept
        {
            physicalIndex = 0;

            if (sequenceIndex >= fPositions.size())
                return false;

            physicalIndex = fPositions[sequenceIndex];
            return true;
        }

        [[nodiscard]] size_t operator[](size_t sequenceIndex) const noexcept
        {
            return fPositions[sequenceIndex];
        }


        // ================================================================
        // applyEdit
        //
        // Update the sequence mapping after one atomic GSUB edit.
        //
        // The edit coordinates describe the buffer immediately BEFORE that
        // edit was performed.
        //
        // Rules:
        //
        //   1. If the anchor is a sequence member, replace that member with
        //      outputCount sequence members.
        //
        //   2. Any other consumed positions that are sequence members are
        //      removed.
        //
        //   3. If the anchor is not a sequence member, its outputs do not
        //      become sequence members.
        //
        //   4. All surviving positions are shifted to their new physical
        //      buffer coordinates.
        //
        // This operation is transactional with respect to this state.
        // ================================================================

        [[nodiscard]] bool applyEdit(const OpenTypeGsubEdit& edit)
        {
            if (!edit)
                return false;

            const std::vector<size_t>& inputs = edit.inputPositions;
            const size_t anchor = inputs[0];
            const size_t insertedCount = edit.outputCount - 1;


            // ------------------------------------------------------------
            // Validate arithmetic needed for generated output positions.
            // ------------------------------------------------------------

            if (insertedCount > std::numeric_limits<size_t>::max() - anchor)
                return false;

            if (insertedCount > std::numeric_limits<size_t>::max() - fPositions.size())
                return false;


            std::vector<size_t> updated;
            updated.reserve(fPositions.size() + insertedCount);

            size_t consumedIndex = 1;
            size_t removedBefore = 0;


            for (size_t oldPosition : fPositions)
            {
                // --------------------------------------------------------
                // Sequence positions before the edit anchor cannot move.
                // --------------------------------------------------------

                if (oldPosition < anchor)
                {
                    updated.push_back(oldPosition);
                    continue;
                }


                // --------------------------------------------------------
                // If the anchor itself belongs to this contextual sequence,
                // all output glyphs become members of the current sequence.
                // --------------------------------------------------------

                if (oldPosition == anchor)
                {
                    for (size_t i = 0; i < edit.outputCount; ++i)
                        updated.push_back(anchor + i);

                    continue;
                }


                // --------------------------------------------------------
                // Count consumed physical positions before this sequence
                // member.
                //
                // inputPositions[0] is the anchor and is not physically
                // removed; only positions 1..N-1 are erased.
                // --------------------------------------------------------

                while (consumedIndex < inputs.size() && inputs[consumedIndex] < oldPosition)
                {
                    ++consumedIndex;
                    ++removedBefore;
                }


                // --------------------------------------------------------
                // This sequence member was itself consumed by the edit.
                // --------------------------------------------------------

                if (consumedIndex < inputs.size() && inputs[consumedIndex] == oldPosition)
                {
                    ++consumedIndex;
                    ++removedBefore;
                    continue;
                }


                // --------------------------------------------------------
                // Multiple substitution inserts outputCount - 1 physical
                // glyphs immediately after the anchor.
                // --------------------------------------------------------

                size_t newPosition = oldPosition;

                if (oldPosition > anchor)
                {
                    if (insertedCount > std::numeric_limits<size_t>::max() - newPosition)
                        return false;

                    newPosition += insertedCount;
                }


                // --------------------------------------------------------
                // Every consumed non-anchor glyph before this position
                // shifts it left by one.
                // --------------------------------------------------------

                if (removedBefore > newPosition)
                    return false;

                newPosition -= removedBefore;
                updated.push_back(newPosition);
            }


            // ------------------------------------------------------------
            // The resulting sequence must remain in strict physical order.
            //
            // Empty is allowed. A nested lookup can consume sequence
            // members while being anchored on a glyph that was not itself
            // part of the outer contextual input sequence.
            // ------------------------------------------------------------

            for (size_t i = 1; i < updated.size(); ++i)
            {
                if (updated[i] <= updated[i - 1])
                    return false;
            }


            fPositions = std::move(updated);
            return true;
        }

    private:
        std::vector<size_t> fPositions{};
    };

} // namespace waavs