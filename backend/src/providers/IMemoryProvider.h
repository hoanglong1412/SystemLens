/**
 * @file    IMemoryProvider.h
 * @brief   Provider interface for physical memory usage.
 */
#pragma once

#include <cstdint>

namespace systemlens::providers {

/** Physical memory at one instant. */
struct MemoryUsage
{
    std::uint64_t totalBytes = 0;
    std::uint64_t usedBytes = 0; ///< total - available.
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
