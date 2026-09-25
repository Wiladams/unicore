// font_resource_file.h
#pragma once

#include "font_resource.h"

#include <filesystem>
#include <fstream>

namespace waavs
{
    namespace fs = std::filesystem;


    // ================================================================
    // Create an interned, absolute source-location string.
    // ================================================================

    inline FontName createNormalizedPath(const fs::path& path)
    {
        std::error_code ec;

        fs::path absolutePath = fs::absolute(path, ec);

        const fs::path& result = ec ? path : absolutePath;

        const std::string text = result.string();

        return WSNameSet::INTERN(text.c_str());
    }

    inline FontName createNormalizedPath(const char* path)
    {
        if (!path || !*path)
            return nullptr;

        return createNormalizedPath(
            fs::path(path));
    }



    inline bool readFontResource(const fs::path& path, FontResource& out)
    {
        out = {};

        std::ifstream file(path, std::ios::binary | std::ios::ate);

        if (!file.is_open())
            return false;

        const std::streamsize size = file.tellg();

        if (size <= 0)
            return false;

        file.seekg(0, std::ios::beg);

        SharedMemBuff buffer;

        if (!buffer.resetFromSize(static_cast<size_t>(size)))
            return false;

        if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
            return false;

        const FontName sourceLocation = createNormalizedPath(path);

        out = FontResource(std::move(buffer), sourceLocation);

        return true;
    }
}
