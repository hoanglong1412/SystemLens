/**
 * @file    IPerformanceDetailService.h
 * @brief   Service interface behind the per-resource performance endpoints
 *          (/api/v1/performance/cpu/cores, memory, disks, networks, gpus), and its result types.
 */
#pragma once

#include "services/monitoring/MetricsSnapshot.h"

#include <vector>

namespace systemlens::services::performance {

/** Usage of one logical processor. */
struct CoreUsage
{
    std::size_t index = 0;
    double usagePercent = 0.0;
    std::vector<double> history;
};

/** Memory figures plus the processes using the most of it. */
struct MemoryDetail
{
    monitoring::MemoryMetrics memory;
    std::vector<monitoring::ProcessMetrics> topProcesses; ///< Highest memory first.
};

class IPerformanceDetailService
{
public:
    virtual ~IPerformanceDetailService() = default;

    /**
     * @brief Usage of every logical processor.
     * @return Cores in index order.
     * @throws std::runtime_error if no metrics snapshot is available.
     */
    [[nodiscard]] virtual std::vector<CoreUsage> getCores() const = 0;

    /**
     * @brief Memory detail.
     * @return Memory figures and the 10 processes using the most memory.
     * @throws std::runtime_error if no metrics snapshot is available.
     */
    [[nodiscard]] virtual MemoryDetail getMemory() const = 0;

    /** @return Every physical disk. @throws std::runtime_error if no metrics snapshot is available. */
    [[nodiscard]] virtual std::vector<monitoring::DiskMetrics> getDisks() const = 0;

    /** @return Every network adapter. @throws std::runtime_error if no metrics snapshot is available. */
    [[nodiscard]] virtual std::vector<monitoring::NetworkMetrics> getNetworks() const = 0;

    /** @return Every graphics adapter. @throws std::runtime_error if no metrics snapshot is available. */
    [[nodiscard]] virtual std::vector<monitoring::GpuMetrics> getGpus() const = 0;
};

} // namespace systemlens::services::performance
