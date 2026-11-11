/**
 * @file    IMemoryProvider.h
 * @brief   Provider interface for physical memory usage.
 */
#pragma once

#include <cstdint>
#include <optional>

namespace systemlens::providers {

/** Physical memory at one instant. */
struct MemoryUsage
{
    std::uint64_t totalBytes = 0;
    std::uint64_t usedBytes = 0; ///< total - available.
    std::uint64_t availableBytes = 0;
    std::optional<std::uint64_t> cachedBytes;     ///< Windows: system cache; Linux: Cached + Buffers.
    std::optional<std::uint64_t> committedBytes;  ///< Windows: CommitTotal; Linux: Committed_AS.
    std::optional<std::uint64_t> commitLimitBytes;
};

class IMemoryProvider
{
public:
    virtual ~IMemoryProvider() = default;

    /**
     * @brief Read the current physical memory usage.
     * @return Total and used bytes.
     * @throws ProviderError on failure.
     */
    [[nodiscard]] virtual MemoryUsage readUsage() const = 0;
};

} // namespace systemlens::providers
