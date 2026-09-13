#pragma once

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include "opentype_bytestream.h"
#include "font_interfaces.h"

namespace waavs
{
    static bool readFileData(const char* filename, std::vector<uint8_t>& data)
    {
        std::ifstream input(filename, std::ios::binary | std::ios::ate);

        if (!input)
            return false;

        const std::streamsize size = input.tellg();

        if (size <= 0)
            return false;

        input.seekg(0, std::ios::beg);

        data.resize(static_cast<size_t>(size));

        return static_cast<bool>(
            input.read(reinterpret_cast<char*>(data.data()), size));
    }

    static bool testReadIndexToLocFormat(const TableRecord& headTable, int16_t& format)
    {
        format = 0;

        // head.indexToLocFormat is Int16 at byte offset 50.
        if (headTable.data.size() < 52)
            return false;

        OpenTypeByteStream stream(headTable.data);

        if (!stream.seek(50))
            return false;

        if (!stream.readInt16(format))
            return false;

        return format == 0 || format == 1;
    }
}