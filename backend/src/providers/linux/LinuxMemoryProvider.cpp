/**
 * @file    LinuxMemoryProvider.cpp
 * @brief   Implementation of LinuxMemoryProvider.
 */
#include "providers/linux/LinuxMemoryProvider.h"

#include "providers/ProviderError.h"
#include "providers/linux/ProcFs.h"

#include <optional>
#include <string_view>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kMemInfoFile = "/proc/meminfo";
constexpr std::string_view kTotalKey = "MemTotal:";
constexpr std::string_view kAvailableKey = "MemAvailable:";
/** /proc/meminfo reports kibibytes. */
constexpr std::uint64_t kBytesPerKib = 1024;

/**
 * @brief Parse one "Key:   123456 kB" line.
 * @param line Line of /proc/meminfo.
 * @param key  Key including the colon.
 * @return Value in bytes, or std::nullopt when the line is not for this key.
 */
std::optional<std::uint64_t> parseMemInfoLine(std::string_view line, std::string_view key)
{
    if (!line.starts_with(key))
    {
        return std::nullopt;
    }
    const auto tokens = procfs::splitWhitespace(line.substr(key.size()));
    if (tokens.empty())
    {
        return std::nullopt;
    }
    const auto kib = procfs::parseUnsigned(tokens.front());
    return kib ? std::optional(*kib * kBytesPerKib) : std::nullopt;
}

} // namespace

/**
 * @brief Read total and used physical memory.
 * @return Usage, where used = MemTotal - MemAvailable (what "free" and Task Manager call "in use").
 * @throws ProviderError if /proc/meminfo is unreadable or lacks the two keys.
 */
MemoryUsage LinuxMemoryProvider::readUsage() const
{
    const std::string content = procfs::readFileOrThrow(kMemInfoFile);
    std::optional<std::uint64_t> total;
    std::optional<std::uint64_t> available;
    for (const std::string_view line : procfs::splitLines(content))
    {
        if (!total)
        {
            total = parseMemInfoLine(line, kTotalKey);
        }
        if (!available)
        {
            available = parseMemInfoLine(line, kAvailableKey);
        }
    }
    if (!total || !available)
    {
        throw ProviderError("MemTotal / MemAvailable missing in /proc/meminfo");
    }
    return MemoryUsage{.totalBytes = *total, .usedBytes = *total - std::min(*available, *total)};
}

} // namespace systemlens::providers::linux_os
