/**
 * @file    IGpuProvider.h
 * @brief   Provider interface for graphics adapters.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::providers {

/** One graphics adapter at one instant. */
struct GpuSample
{
    std::string id;    ///< Stable identifier, e.g. "gpu0".
    std::string name;  ///< Display name, e.g. "GPU 0".
    std::string model; ///< Adapter model, e.g. "Intel(R) UHD Graphics 630".
    std::optional<double> utilizationPercent; ///< 0..100; nullopt when the driver does not report it.
    std::optional<double> temperatureCelsius;
    std::optional<std::uint64_t> memoryTotalBytes;
    std::optional<std::uint64_t> memoryUsedBytes; ///< Only where the driver reports it.
};

class IGpuProvider
{
public:
    virtual ~IGpuProvider() = default;

    /**
     * @brief Enumerate hardware graphics adapters. Meant to be called once per sampling interval:
     *        implementations may measure utilization between two consecutive calls.
     * @return Adapters in display order; empty when none is visible.
     * @throws ProviderError if the adapters cannot be enumerated.
     */
    [[nodiscard]] virtual std::vector<GpuSample> readGpus() = 0;
};

} // namespace systemlens::providers
