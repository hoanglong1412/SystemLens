/**
 * @file    DynamicLibrary.h
 * @brief   Shared library loaded at runtime by absolute path: symbol lookup and unload.
 *
 * OS-independent declaration; the implementation is selected by CMake:
 *   platform/windows/DynamicLibrary.cpp (LoadLibraryExW), platform/posix/DynamicLibrary.cpp (dlopen).
 */
#pragma once

#include <filesystem>
#include <string>

namespace systemlens::platform {

class DynamicLibrary
{
public:
    explicit DynamicLibrary(const std::filesystem::path& path);
    ~DynamicLibrary();

    // Owns an OS handle: copying would unload the library twice.
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    DynamicLibrary(DynamicLibrary&&) = delete;
    DynamicLibrary& operator=(DynamicLibrary&&) = delete;

    void* findSymbol(const char* name) const;
    const std::filesystem::path& path() const noexcept;

private:
    std::filesystem::path m_path;
    void* m_handle = nullptr; ///< HMODULE on Windows, dlopen() handle on POSIX.
};

} // namespace systemlens::platform
