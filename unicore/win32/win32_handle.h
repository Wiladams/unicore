#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace waavs
{
    class WinHandle
    {
        HANDLE fHandle{};

    public:
        WinHandle() noexcept = default;
        explicit WinHandle(HANDLE handle) noexcept : fHandle(handle) {}

        ~WinHandle() noexcept
        {
            if (fHandle && fHandle != INVALID_HANDLE_VALUE)
                ::CloseHandle(fHandle);
        }

        WinHandle(const WinHandle&) = delete;
        WinHandle& operator=(const WinHandle&) = delete;

        WinHandle(WinHandle&& other) noexcept
            : fHandle(other.fHandle)
        {
            other.fHandle = nullptr;
        }

        WinHandle& operator=(WinHandle&& other) noexcept
        {
            if (this == &other)
                return *this;

            if (fHandle && fHandle != INVALID_HANDLE_VALUE)
                ::CloseHandle(fHandle);

            fHandle = other.fHandle;
            other.fHandle = nullptr;

            return *this;
        }

        HANDLE get() const noexcept { return fHandle; }

        HANDLE release() noexcept
        {
            HANDLE result = fHandle;
            fHandle = nullptr;
            return result;
        }

        explicit operator bool() const noexcept
        {
            return fHandle && fHandle != INVALID_HANDLE_VALUE;
        }
    };
}