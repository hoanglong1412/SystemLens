/**
 * @file    ExecutablePath.cpp (macos)
 * @brief   macOS implementation of executableDirectory() (_NSGetExecutablePath + canonical path).
 *          Written but not built in this change (no macOS build machine).
 */
#include "platform/ExecutablePath.h"

#include <mach-o/dyld.h>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace systemlens::platform {

/**
 * @brief Directory that contains the running executable.
 * @return Absolute directory path with symlinks resolved.
 * @throws std::runtime_error when the path cannot be read.
 */
std::filesystem::path executableDirectory()
{
    // First call with an empty buffer only reports the required size.
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0)
    {
        throw std::runtime_error("_NSGetExecutablePath failed");
    }
    // size counts the terminating NUL; keep only the characters before it.
    buffer.resize(std::strlen(buffer.c_str()));

    // The returned path may be relative or go through a symlink; canonical() resolves both.
    return std::filesystem::canonical(std::filesystem::path(buffer)).parent_path();
}

} // namespace systemlens::platform
