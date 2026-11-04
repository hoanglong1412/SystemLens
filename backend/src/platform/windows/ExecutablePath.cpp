/**
 * @file    ExecutablePath.cpp (windows)
 * @brief   Windows implementation of executableDirectory() (GetModuleFileNameW).
 */
#include "platform/ExecutablePath.h"

#include <windows.h>

#include <stdexcept>
#include <string>

namespace systemlens::platform {

namespace {

constexpr DWORD kInitialPathLength = MAX_PATH;
// Long paths (\\?\ prefix) are limited to 32767 wide characters.
constexpr DWORD kMaxPathLength = 32768;

} // namespace

/**
 * @brief Directory that contains the running executable.
 * @return Absolute directory path.
 * @throws std::runtime_error when the path cannot be read.
 */
std::filesystem::path executableDirectory()
{
    // GetModuleFileNameW truncates silently when the buffer is too small, so grow until it fits.
    for (DWORD size = kInitialPathLength; size <= kMaxPathLength; size *= 2)
    {
        std::wstring buffer(size, L'\0');
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), size);
        if (length == 0)
        {
            throw std::runtime_error("GetModuleFileNameW failed (error " + std::to_string(GetLastError()) + ")");
        }
        if (length < size)
        {
            buffer.resize(length);
            return std::filesystem::path(buffer).parent_path();
        }
    }
    throw std::runtime_error("executable path is too long");
}

} // namespace systemlens::platform
