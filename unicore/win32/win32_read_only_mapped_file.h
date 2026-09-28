#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "lang_span.h"
#include "win32_handle.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <filesystem>
#include <limits>


namespace waavs
{
    class ReadOnlyMappedFile
    {
        const void* fData{};
        std::size_t fSize{};
        HANDLE fMapHandle{};


        ReadOnlyMappedFile(HANDLE maphandle, const void* data, std::size_t size) noexcept
            : fData(data)
            , fSize(size)
            , fMapHandle(maphandle)
        {}


        void close() noexcept
        {
            if (fData) {
                ::UnmapViewOfFile(fData);
                fData = nullptr;
            }

            if (fMapHandle) {
                ::CloseHandle(fMapHandle);
                fMapHandle = nullptr;
            }

            fSize = 0;
        }

    public:
        ~ReadOnlyMappedFile() noexcept { close(); }

        // don't want copy semantics for this class, only move semantics
        ReadOnlyMappedFile(const ReadOnlyMappedFile&) = delete;
        ReadOnlyMappedFile& operator=(const ReadOnlyMappedFile&) = delete;

        // move semantics
        ReadOnlyMappedFile(ReadOnlyMappedFile&& other) noexcept
            : fData(other.fData)
            , fSize(other.fSize)
            , fMapHandle(other.fMapHandle)
        {
            other.fData = nullptr;
            other.fSize = 0;
            other.fMapHandle = nullptr;
        }

        ReadOnlyMappedFile& operator=(ReadOnlyMappedFile&& other) noexcept
        {
            if (this == &other)
                return *this;

            close();

            fData = other.fData;
            fSize = other.fSize;
            fMapHandle = other.fMapHandle;

            other.fData = nullptr;
            other.fSize = 0;
            other.fMapHandle = nullptr;

            return *this;
        }

        const uint8_t* data() const noexcept 
        { 
            return static_cast<const uint8_t *>(fData); 
        }
        std::size_t size() const noexcept { return fSize; }
        bool empty() const noexcept { return fSize == 0; }

        ByteSpan bytes() const noexcept
        {
            return ByteSpan(static_cast<const uint8_t*>(fData), fSize);
        }


        static std::optional<ReadOnlyMappedFile> open(const std::filesystem::path& filename)
        {
            WinHandle file(::CreateFileW(
                filename.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr));

            if (!file)
                return std::nullopt;

            LARGE_INTEGER fileSize{};
            if (!::GetFileSizeEx(file.get(), &fileSize)) {
                return std::nullopt;
            }

            if (fileSize.QuadPart < 0 ||
                static_cast<uint64_t>(fileSize.QuadPart) > std::numeric_limits<size_t>::max()) 
            {
                return std::nullopt;
            }

            size_t size = static_cast<size_t>(fileSize.QuadPart);

            // A zero-length file cannot have a Windows file mapping,
            // but it is still a successfully opened file.
            if (size == 0) {
                return ReadOnlyMappedFile(nullptr, nullptr, 0);
            }

            WinHandle mapping(::CreateFileMappingW(
                file.get(),
                nullptr,
                PAGE_READONLY,
                0,
                0,
                nullptr));

            if (!mapping)
                return std::nullopt;



            const void* data = ::MapViewOfFile(mapping.get(),FILE_MAP_READ,0,0,0);

            if (!data) {
                return std::nullopt;
            }


            return ReadOnlyMappedFile(mapping.release(), data, size);
        }

        private:

    };
}
