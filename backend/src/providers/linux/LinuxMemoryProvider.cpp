/**
 * @file    LinuxMemoryProvider.cpp
 * @brief   Implementation of LinuxMemoryProvider.
 */
#include "providers/linux/LinuxMemoryProvider.h"

#include "providers/ProviderError.h"
#include "providers/linux/ProcFs.h"

#include <algorithm>
#include <optional>
#include <string_view>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kMemInfoFile = "/proc/meminfo";
constexpr std::string_view kTotalKey = "MemTotal:";
constexpr std::string_view kAvailableKey = "MemAvailable:";
constexpr std::string_view kCachedKey = "Cached:";
constexpr std::string_view kBuffersKey = "Buffers:";
constexpr std::string_view kCommittedKey = "Committed_AS:";
constexpr std::string_view kCommitLimitKey = "CommitLimit:";
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
 * @brief Read memory usage and commit figures.
 * @return Usage, where used = MemTotal - MemAvailable (what "free" and Task Manager call "in use").
 * @throws ProviderError if /proc/meminfo is unreadable or lacks MemTotal / MemAvailable.
 */
MemoryUsage LinuxMemoryProvider::readUsage() const
{
    const std::string content = procfs::readFileOrThrow(kMemInfoFile);
    std::optional<std::uint64_t> total;
    std::optional<std::uint64_t> available;
    std::optional<std::uint64_t> cached;
    std::optional<std::uint64_t> buffers;
    std::optional<std::uint64_t> committed;
    std::optional<std::uint64_t> commitLimit;
    for (const std::string_view line : procfs::splitLines(content))
    {
        // A line matches at most one key, so the first hit decides.
        if (const auto value = parseMemInfoLine(line, kTotalKey)) total = value;
        else if (const auto value = parseMemInfoLine(line, kAvailableKey)) available = value;
        else if (const auto value = parseMemInfoLine(line, kCachedKey)) cached = value;
        else if (const auto value = parseMemInfoLine(line, kBuffersKey)) buffers = value;
        else if (const auto value = parseMemInfoLine(line, kCommittedKey)) committed = value;
        else if (const auto value = parseMemInfoLine(line, kCommitLimitKey)) commitLimit = value;
    }
    if (!total || !available)
    {
        throw ProviderError("MemTotal / MemAvailable missing in /proc/meminfo");
    }
    const std::uint64_t availableBytes = std::min(*available, *total);
    return MemoryUsage{
        .totalBytes = *total,
        .usedBytes = *total - availableBytes,
        .availableBytes = availableBytes,
        .cachedBytes = cached ? std::optional(*cached + buffers.value_or(0)) : std::nullopt,
        .committedBytes = committed,
        .commitLimitBytes = commitLimit,
    };
}

} // namespace systemlens::providers::linux_os
