// font_resource_file.h
#pragma once

#include "font_resource.h"

#include <filesystem>

namespace waavs
{
    namespace fs = std::filesystem;




    // ================================================================
    // Read a font resource from a file.
    // ================================================================
    inline bool readFontResource(const fs::path& path, FontResource& out)
    {
        out = {};

        auto mapped = ReadOnlyMappedFile::open(path);

        if (!mapped || mapped->empty())
            return false;

        auto owner = std::make_shared<ReadOnlyMappedFile>(std::move(*mapped));

        const FontName sourceLocation = createNormalizedPath(path);

        out = FontResource(owner->bytes(), owner, sourceLocation);

        return true;
    }



}
