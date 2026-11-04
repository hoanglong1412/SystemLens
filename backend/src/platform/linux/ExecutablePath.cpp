/**
 * @file    ExecutablePath.cpp (linux)
 * @brief   Linux implementation of executableDirectory() (readlink /proc/self/exe).
 */
#include "platform/ExecutablePath.h"

#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

namespace systemlens::platform {

namespace {

constexpr const char* kSelfExeLink = "/proc/self/exe";
constexpr std::size_t kInitialPathLength = 256;
constexpr std::size_t kMaxPathLength = 65536;

} // namespace

/**
 * @brief Directory that contains the running executable.
 * @return Absolute directory path (symlinks resolved by the kernel).
 * @throws std::runtime_error when the path cannot be read.
 */
std::filesystem::path executableDirectory()
{
    // readlink() does not report truncation, only a full buffer, so grow until the result is shorter.
    for (std::size_t size = kInitialPathLength; size <= kMaxPathLength; size *= 2)
    {
        std::string buffer(size, '\0');
        const ssize_t length = readlink(kSelfExeLink, buffer.data(), buffer.size());
        if (length < 0)
        {
            throw std::runtime_error(std::string("readlink ") + kSelfExeLink + " failed: " + std::strerror(errno));
        }
        if (static_cast<std::size_t>(length) < size)
        {
            buffer.resize(static_cast<std::size_t>(length));
            return std::filesystem::path(buffer).parent_path();
        }
    }
    throw std::runtime_error("executable path is too long");
}

} // namespace systemlens::platform
