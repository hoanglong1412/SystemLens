/**
 * @file    PerformanceService.cpp
 * @brief   Implementation of PerformanceService.
 */
#include "services/performance/PerformanceService.h"

#include <algorithm>
#include <stdexcept>

namespace systemlens::services::performance {

namespace {

constexpr double kMaxPercent = 100.0;
/** Smallest full scale of a network sparkline, so an idle adapter does not amplify noise. */
constexpr double kMinNetworkScaleBitsPerSecond = 100'000.0;

/**
 * @brief Name the media type of a disk.
 * @param kind Disk kind.
 * @return "SSD", "HDD", or std::nullopt when unknown.
 */
std::optional<std::string> describeDiskKind(providers::DiskKind kind)
{
    switch (kind)
    {
    case providers::DiskKind::Ssd:
        return "SSD";
    case providers::DiskKind::Hdd:
        return "HDD";
    case providers::DiskKind::Unknown:
        break;
    }
    return std::nullopt;
}

/**
 * @brief Scale a throughput history to percentages of its own peak.
 * @param bitsPerSecond Throughput samples.
 * @return Same number of samples in 0..100; full scale is the peak, at least kMinNetworkScaleBitsPerSecond.
 */
std::vector<double> scaleToPeak(const std::vector<double>& bitsPerSecond)
{
    const double peak = bitsPerSecond.empty() ? 0.0 : *std::ranges::max_element(bitsPerSecond);
    const double scale = std::max(peak, kMinNetworkScaleBitsPerSecond);
    std::vector<double> percents(bitsPerSecond.size());
    std::ranges::transform(bitsPerSecond, percents.begin(),
                           [scale](double value) { return value / scale * kMaxPercent; });
    return percents;
}

} // namespace

/**
 * @brief Create the service.
 * @param metrics Source of metrics snapshots. Must not be null.
 * @throws std::invalid_argument if metrics is null.
 */
PerformanceService::PerformanceService(std::shared_ptr<const monitoring::IMetricsSource> metrics)
    : m_metrics(std::move(metrics))
{
    if (!m_metrics)
    {
        throw std::invalid_argument("PerformanceService: metrics source must not be null");
    }
}

/**
 * @brief Build the resource list from the latest snapshot.
 * @return CPU, memory, then every disk, network adapter and GPU.
 * @throws std::runtime_error if the metrics source has no valid snapshot.
 */
std::vector<ResourceSummary> PerformanceService::getResources() const
{
    const auto snapshot = m_metrics->latest();
    std::vector<ResourceSummary> resources;
    resources.reserve(2 + snapshot->disks.size() + snapshot->networks.size() + snapshot->gpus.size());

    ResourceSummary cpu;
    cpu.id = "cpu";
    cpu.kind = ResourceKind::Cpu;
    cpu.name = "CPU";
    cpu.percent = snapshot->cpu.usagePercent;
    cpu.history = snapshot->cpu.usageHistory;
    cpu.frequencyMhz = snapshot->cpu.frequencyMhz;
    resources.push_back(std::move(cpu));

    ResourceSummary memory;
    memory.id = "memory";
    memory.kind = ResourceKind::Memory;
    memory.name = "Memory";
    memory.percent = snapshot->memory.usedPercent;
    memory.history = snapshot->memory.usedPercentHistory;
    memory.usedBytes = snapshot->memory.usedBytes;
    memory.totalBytes = snapshot->memory.totalBytes;
    resources.push_back(std::move(memory));

    for (const monitoring::DiskMetrics& disk : snapshot->disks)
    {
        ResourceSummary resource;
        resource.id = disk.id;
        resource.kind = ResourceKind::Disk;
        resource.name = disk.name;
        resource.description = describeDiskKind(disk.kind);
        resource.percent = disk.activePercent;
        resource.history = disk.activePercentHistory;
        resources.push_back(std::move(resource));
    }

    for (const monitoring::NetworkMetrics& adapter : snapshot->networks)
    {
        const bool isWifi = adapter.kind == providers::NetworkKind::Wifi;
        ResourceSummary resource;
        resource.id = adapter.id;
        resource.kind = isWifi ? ResourceKind::Wifi : ResourceKind::Ethernet;
        resource.name = isWifi ? "Wi-Fi" : "Ethernet";
        resource.description = adapter.name;
        resource.history = scaleToPeak(adapter.throughputHistory);
        resource.sendBitsPerSecond = adapter.sendBitsPerSecond;
        resource.receiveBitsPerSecond = adapter.receiveBitsPerSecond;
        resources.push_back(std::move(resource));
    }

    for (const monitoring::GpuMetrics& gpu : snapshot->gpus)
    {
        ResourceSummary resource;
        resource.id = gpu.id;
        resource.kind = ResourceKind::Gpu;
        resource.name = gpu.name;
        resource.description = gpu.model;
        resource.percent = gpu.utilizationPercent;
        resource.history = gpu.utilizationHistory;
        resource.temperatureCelsius = gpu.temperatureCelsius;
        resources.push_back(std::move(resource));
    }
    return resources;
}

/**
 * @brief Build the CPU screen data from the latest snapshot.
 * @return CPU performance; uptime is measured at the time of the snapshot.
 * @throws std::runtime_error if the metrics source has no valid snapshot.
 */
CpuPerformance PerformanceService::getCpuPerformance() const
{
    const auto snapshot = m_metrics->latest();
    return CpuPerformance{
        .info = snapshot->cpuInfo,
        .cpu = snapshot->cpu,
        .processCount = snapshot->counts.processes,
        .threadCount = snapshot->counts.threads,
        .handleCount = snapshot->counts.handles,
        .bootTime = snapshot->bootTime,
        .uptime = std::chrono::duration_cast<std::chrono::seconds>(snapshot->takenAt - snapshot->bootTime),
        .topProcesses = snapshot->topCpuProcesses,
    };
}

} // namespace systemlens::services::performance
