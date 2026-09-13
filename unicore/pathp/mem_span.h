// mem_span.h

#pragma once


#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>

//
// MemSpan
// 
// A core type for representing a contiguous sequence of bytes.
// As of C++ 20, there is std::span<>, and that would be a good 
// choice, but it is not yet widely supported, and forces a jump
// to C++20 besides.
// 
// The MemSpan, is just like 'span' and 'view' objects
// it does not "own" the memory, it just points at it.
// It is used as a stand-in for various data representations.
// A key aspect of the MemSpan is its ability to be used
// as a 'cursor' to traverse the data it points to.
//

namespace waavs 
{

    struct MemSpan final
    {
    private:
        const uint8_t* fStart{ nullptr };
        const uint8_t* fEnd{ nullptr };

    public:
        static const MemSpan& null() noexcept 
        {
            static MemSpan nullSpan{};
            return nullSpan;
        }

        // Constructors
        constexpr MemSpan() noexcept = default;

        // Construct from start and end pointers
        constexpr MemSpan(const uint8_t* start, const uint8_t* end) noexcept 
            : fStart(start)
            , fEnd(end) {}
        
        // Construct from a pointer and size
        constexpr MemSpan(const uint8_t * start, size_t sz) noexcept
            : fStart(start)
            , fEnd(start ? start + sz : nullptr)
        {
        }

        // Construct from a null-terminated C string
        // Error:  If there is no null terminator, this will read past the end of the buffer
        MemSpan(const char* cstr) noexcept
        {
            if (!cstr)
            {
                fStart = nullptr;
                fEnd = nullptr;
                return;
            }

            fStart = reinterpret_cast<const uint8_t*>(cstr);
            fEnd = fStart + std::strlen(cstr);
        }

        //~MemSpan() = default;


        constexpr void reset() { fStart = nullptr; fEnd = nullptr; }
        
        constexpr void resetPointers(const uint8_t * start, const uint8_t * end) noexcept
        {
            fStart = start;
            fEnd = end;
        }
        constexpr void resetStart(const uint8_t * start) noexcept { fStart = start; }
        constexpr void resetEnd(const uint8_t* end) noexcept {fEnd = end;}

        constexpr void resetFromSize(const void *data, size_t sz) noexcept
        {
            fStart = static_cast<const uint8_t*>(data);
            fEnd = fStart + sz;
        }
        


        // setting up for a range-based for loop
        // not actually that useful, as it's just memory traversal
        // but, having data() and size() hides the internals
        constexpr const uint8_t* data() const noexcept { return fStart; }
        constexpr size_t size() const noexcept { return (fStart && fEnd >= fStart) ? size_t(fEnd - fStart) : 0; }

        constexpr const uint8_t* begin() const noexcept { return fStart; }
        constexpr const uint8_t* end() const noexcept { return fEnd; }
        constexpr bool empty() const noexcept { return size() == 0; }


        // Type conversions
        explicit constexpr operator bool() const noexcept { return !empty(); }

        // get value of character at fStart, like a 'peek' operation
        // If the MemSpan is currently empty, these will return 0, rather than 
        // throwing an exception, so check size before calling if that's necessary
        constexpr uint8_t operator*() const noexcept
        {
            return (fStart && fStart < fEnd) ? *fStart : 0;
        }


        // Create a MemSpan beginning at startAt and extending
        // through the end of the current span.
        // If startAt is beyond the end, returns an empty span
        // positioned at the end.
        constexpr MemSpan subSpan(size_t startAt) const noexcept
        {
            if (!fStart)
                return {};

            const size_t n = size();
            const size_t off = startAt < n ? startAt : n;

            return MemSpan(fStart + off, n - off);
        }

        // subSpan()
        // 
        // Create a MemSpan that is a view on the current span
        // If the requested position plus size is greater than the amount
        // of span remaining at that position, the size will be truncated 
        // to the amount remaining from the requested position.
        // So, it's more like an intersection of the desired subspan
        // and the current span.
        constexpr MemSpan subSpan(size_t startAt, size_t sz) const noexcept
        {
            if (!fStart)
                return {};

            const size_t n = size();
            const size_t off = startAt < n ? startAt : n;
            const size_t len = sz < (n - off) ? sz : (n - off);
            
            return MemSpan(fStart + off, len);
        }

        constexpr MemSpan take(size_t n) const noexcept
        {
            return subSpan(0, n);
        }

        // advance()
        // 
        // advance the start pointer the specified number of entries
        // constrain to end 
        // Protect against null pointers
        constexpr MemSpan& advance(size_t n) noexcept
        {
            if (!fStart || !fEnd)
                return *this;

            const size_t remaining = size();
            fStart += (n < remaining) ? n : remaining;
            return *this;
        }

        constexpr MemSpan& advanceToEnd() noexcept
        {
            fStart = fEnd;
            return *this;
        }

        constexpr MemSpan& operator+=(size_t n) noexcept  {  return advance(n); }

        constexpr MemSpan& operator++() noexcept { return advance(1); }
        //constexpr MemSpan operator++(int) noexcept
        //{
        //    MemSpan tmp = *this;
        //    advance(1);
        //    return tmp;
        //}




        // Array access
        uint8_t& operator[](size_t i) noexcept { return const_cast<uint8_t&>(fStart[i]); }
        const uint8_t& operator[](size_t i) const noexcept { return fStart[i]; }


        // BUGBUG - not sure these should be used any more
        // favoring interned strings is probably a better approach
        // operators for comparison
        // 
        // operator==;
        // operator!=;
        // operator<=;
        // operator>=;
        
        // isEqual()
        // A pointer comparison
        bool isEqual(const MemSpan& b) const noexcept
        {
            return fStart == b.fStart && size() == b.size();
        }

        bool equivalent(const MemSpan& b) const noexcept
        {
            const size_t n = size();
            if (n != b.size())
                return false;

            if (n == 0)
                return true;

            if (!fStart || !b.fStart)
                return false;

            return std::memcmp(fStart, b.fStart, n) == 0;
        }
        
        // operator==
        // Perform a full content comparison of the two spans
        bool operator==(const MemSpan& b) const noexcept
        {
            return equivalent(b);
        }

        bool operator==(const char* b) const noexcept
        {
            if (!b)
                return false;

            return equivalent(MemSpan(b));
        }

        bool operator!=(const MemSpan& other) const noexcept
        {
            return !(*this == other);
        }

        /*
        bool operator<(const MemSpan& b) const noexcept
        {
            size_t minSize = size() < b.size() ? size() : b.size();
            int cmp = memcmp(fStart, b.fStart, minSize);
            return (cmp < 0) || (cmp == 0 && size() < b.size());
        }


        bool operator>(const MemSpan& b) const noexcept
        {
            size_t minSize = size() < b.size() ? size() : b.size();
            int cmp = memcmp(fStart, b.fStart, minSize);
            return (cmp > 0) || (cmp == 0 && size() > b.size());
        }


        bool operator<=(const MemSpan& b) const noexcept
        {
            size_t minSize = size() < b.size() ? size() : b.size();
            int cmp = memcmp(fStart, b.fStart, minSize);
            return (cmp < 0) || (cmp == 0 && size() <= b.size());
        }

        bool operator>=(const MemSpan& b) const noexcept
        {
            size_t minSize = size() < b.size() ? size() : b.size();
            int cmp = memcmp(fStart, b.fStart, minSize);
            return (cmp > 0) || (cmp == 0 && size() >= b.size());
        }
        */
        // -----------------------------------------
        // Static factory methods for convenience
        // -----------------------------------------
        static  constexpr MemSpan fromPointers(const uint8_t* startAt, const uint8_t* endAt) noexcept
        {
            MemSpan bs(startAt, endAt);
            return bs;
        }

        static inline MemSpan fromPointerAndSize(const uint8_t* start, size_t sz)
        {
            MemSpan bs;
            bs.fStart = start;
            bs.fEnd = start + sz;
            return bs;
        }
    };

    //ASSERT_MEMCPY_SAFE(MemSpan);
}





