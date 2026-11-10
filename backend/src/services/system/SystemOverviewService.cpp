/**
 * @file    SystemOverviewService.cpp
 * @brief   Implementation of SystemOverviewService.
 */
#include "services/system/SystemOverviewService.h"

#include <algorithm>
#include <stdexcept>

namespace systemlens::services::system {

/**
 * @brief Create the service.
 * @param metrics Source of metrics snapshots. Must not be null.
 * @throws std::invalid_argument if metrics is null.
 */
SystemOverviewService::SystemOverviewService(std::shared_ptr<const monitoring::IMetricsSource> metrics)
    : m_metrics(std::move(metrics))
{
    if (!m_metrics)
    {
        throw std::invalid_argument("SystemOverviewService: metrics source must not be null");
    }
}

/**
 * @brief Build the overview from the latest snapshot.
 * @return Overview; uptime is measured at the time of the snapshot.
 * @throws std::runtime_error if the metrics source has no valid snapshot.
 */
SystemOverview SystemOverviewService::getOverview() const
{
    const auto snapshot = m_metrics->latest();

    SystemOverview overview;
    overview.machine = snapshot->identity;
    overview.bootTime = snapshot->bootTime;
    overview.uptime = std::chrono::duration_cast<std::chrono::seconds>(snapshot->takenAt - snapshot->bootTime);
    overview.sampleInterval = snapshot->sampleInterval;
    overview.processCount = snapshot->counts.processes;
    overview.serviceCount = snapshot->counts.services;
    overview.cpuPercent = snapshot->cpu.usagePercent;
    overview.memoryPercent = snapshot->memory.usedPercent;

    for (const monitoring::DiskMetrics& disk : snapshot->disks)
    {
        if (disk.activePercent)
        {
            overview.diskPercent = std::max(overview.diskPercent.value_or(0.0), *disk.activePercent);
        }
    }
    for (const monitoring::NetworkMetrics& adapter : snapshot->networks)
    {
        overview.sendBitsPerSecond += adapter.sendBitsPerSecond;
        overview.receiveBitsPerSecond += adapter.receiveBitsPerSecond;
    }
    if (snapshot->topMemoryProcess)
    {
        overview.topMemoryProcess = TopMemoryProcess{
            .name = snapshot->topMemoryProcess->name,
            .memoryBytes = snapshot->topMemoryProcess->memoryBytes,
        };
    }
    return overview;
}

} // namespace systemlens::services::system
