// font_resource.h
#pragma once

#include "lang_memory.h"
#include "lang_span.h"
#include "core_nametable.h"
#include "font_interfaces.h"

namespace waavs
{
    // Retained bytes for one font resource.
    //
    // FontResource knows nothing about TTF, OTF, TTC, WOFF, or font faces.
    // It only owns/retains the original resource bytes and source identity.

    class FontResource
    {
    private:
        SharedMemBuff fData;
        FontName fSourceLocation{ nullptr };

    public:
        FontResource() = default;

        FontResource(SharedMemBuff data, FontName sourceLocation = nullptr) noexcept
            : fData(std::move(data))
            , fSourceLocation(sourceLocation)
        {}

        [[nodiscard]] bool isValid() const noexcept
        {
            return bool(fData);
        }

        explicit operator bool() const noexcept
        {
            return isValid();
        }

        [[nodiscard]] ByteSpan data() const noexcept
        {
            return ByteSpan(fData.data(), fData.size());
        }

        [[nodiscard]] size_t size() const noexcept
        {
            return fData.size();
        }

        [[nodiscard]] FontName sourceLocation() const noexcept
        {
            return fSourceLocation;
        }

        [[nodiscard]] const SharedMemBuff& storage() const noexcept
        {
            return fData;
        }
    };


    inline bool makeFontResource(const ByteSpan& data, FontResource& out, FontName sourceLocation = nullptr)
    {
        out = {};

        if (data.empty())
            return false;

        SharedMemBuff buffer(data.size());

        if (!buffer)
            return false;

        std::memcpy(buffer.data(), data.begin(), data.size());

        out = FontResource(std::move(buffer), sourceLocation);
        return true;
    }
}