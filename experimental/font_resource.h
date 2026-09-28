// font_resource.h
#pragma once

#include "lang_memory.h"
#include "lang_span.h"
#include "core_nametable.h"
#include "font_interfaces.h"
#include "read_only_mapped_file.h"

#include <memory>
#include <filesystem>



namespace waavs
{
    namespace fs = std::filesystem;


    // Retained bytes for one font resource.
    //
    // FontResource knows nothing about TTF, OTF, TTC, WOFF, or font faces.
    // It only owns/retains the original resource bytes and source identity.

    class FontResource
    {
        ByteSpan fData;
        std::shared_ptr< const void> fOwner;
        FontName fSourceLocation{ nullptr };

    public:
        FontResource() = default;

        template<typename T>
        FontResource(ByteSpan data, std::shared_ptr<T> owner, FontName sourceLocation = nullptr) noexcept
            : fData(std::move(data))
            , fOwner(std::move(owner))
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
            return fData;
        }

        [[nodiscard]] size_t size() const noexcept
        {
            return fData.size();
        }

        [[nodiscard]] FontName sourceLocation() const noexcept
        {
            return fSourceLocation;
        }

    };


}