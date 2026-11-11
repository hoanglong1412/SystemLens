/**
 * @file    WindowsGpuProvider.h
 * @brief   Windows IGpuProvider: DXGI for the adapters, PDH "GPU Engine" counters for utilization.
 */
#pragma once

#include "providers/IGpuProvider.h"
#include "providers/windows/PdhQuery.h"

#include <cstdint>

namespace systemlens::providers::windows_os {

class WindowsGpuProvider final : public IGpuProvider
{
public:
    WindowsGpuProvider();
    [[nodiscard]] std::vector<GpuSample> readGpus() override;

private:
    /** A hardware adapter found at construction. */
    struct Adapter
    {
        std::string model;
        std::uint64_t dedicatedMemoryBytes = 0; ///< Video memory of the adapter; 0 for integrated GPUs.
        std::uint64_t luid = 0; ///< Locally unique id that the GPU Engine counter instances carry.
    };

    std::vector<Adapter> m_adapters;
    PdhQuery m_query;
    std::optional<PdhQuery::Counter> m_engineUtilization;
};

} // namespace systemlens::providers::windows_os
