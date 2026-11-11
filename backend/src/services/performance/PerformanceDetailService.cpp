/**
 * @file    PerformanceDetailService.cpp
 * @brief   Implementation of PerformanceDetailService.
 */
#include "services/performance/PerformanceDetailService.h"

#include "services/monitoring/MetricsCollector.h"

#include <algorithm>
#include <stdexcept>

namespace systemlens::services::performance {

/**
 * @brief Create the service.
 * @param metrics Source of metrics snapshots. Must not be null.
 * @throws std::invalid_argument if metrics is null.
 */
PerformanceDetailService::PerformanceDetailService(std::shared_ptr<const monitoring::IMetricsSource> metrics)
    : m_metrics(std::move(metrics))
{
    if (!m_metrics)
    {
        throw std::invalid_argument("PerformanceDetailService: metrics source must not be null");
    }
}

/**
 * @brief Pair every core's current usage with its history.
 * @return Cores in index order.
 * @throws std::runtime_error if the metrics source has no valid snapshot.
 */
std::vector<CoreUsage> PerformanceDetailService::getCores() const
{
    const auto snapshot = m_metrics->latest();
    const monitoring::CpuMetrics& cpu = snapshot->cpu;
    std::vector<CoreUsage> cores;
    cores.reserve(cpu.coreUsagePercent.size());
    for (std::size_t index = 0; index < cpu.coreUsagePercent.size(); ++index)
    {
        cores.push_back(CoreUsage{
            .index = index,
            .usagePercent = cpu.coreUsagePercent[index],
            .history = index < cpu.coreUsageHistory.size() ? cpu.coreUsageHistory[index] : std::vector<double>{},
        });
    }
    return cores;
}

/**
 * @brief Memory figures with the processes using the most memory.
 * @return Memory detail.
 * @throws std::runtime_error if the metrics source has no valid snapshot.
 */
MemoryDetail PerformanceDetailService::getMemory() const
{
    const auto snapshot = m_metrics->latest();
    MemoryDetail detail{.memory = snapshot->memory, .topProcesses = snapshot->processes};
    const auto count = static_cast<std::ptrdiff_t>(std::min(monitoring::kTopProcessCount, detail.topProcesses.size()));
    std::ranges::partial_sort(detail.topProcesses, detail.topProcesses.begin() + count,
                              [](const monitoring::ProcessMetrics& a, const monitoring::ProcessMetrics& b) {
                                  return a.memoryBytes > b.memoryBytes;
                              });
    detail.topProcesses.resize(static_cast<std::size_t>(count));
    return detail;
}

/** @brief Every disk of the latest snapshot. @throws std::runtime_error if no valid snapshot. */
std::vector<monitoring::DiskMetrics> PerformanceDetailService::getDisks() const
{
    return m_metrics->latest()->disks;
}

/** @brief Every network adapter of the latest snapshot. @throws std::runtime_error if no valid snapshot. */
std::vector<monitoring::NetworkMetrics> PerformanceDetailService::getNetworks() const
{
    return m_metrics->latest()->networks;
}

/** @brief Every graphics adapter of the latest snapshot. @throws std::runtime_error if no valid snapshot. */
std::vector<monitoring::GpuMetrics> PerformanceDetailService::getGpus() const
{
    return m_metrics->latest()->gpus;
}

} // namespace systemlens::services::performance
