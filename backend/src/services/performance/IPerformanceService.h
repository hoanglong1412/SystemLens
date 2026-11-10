/**
 * @file    IPerformanceService.h
 * @brief   Service interface behind the performance endpoints (/api/v1/performance/...), and its result types.
 */
#pragma once

#include "providers/ICpuProvider.h"
#include "services/monitoring/MetricsSnapshot.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::services::performance {

/** Kind of a monitored resource (one row of the sidebar). */
enum class ResourceKind
{
    Cpu,
    Memory,
    Disk,
    Ethernet,
    Wifi,
    Gpu,
};

/** One monitored resource. Fields that do not apply to the kind are nullopt. */
struct ResourceSummary
{
    std::string id;
    ResourceKind kind = ResourceKind::Cpu;
    std::string name;
    std::optional<std::string> description;
    std::optional<double> percent;
    std::vector<double> history; ///< 0..100, oldest first.
    std::optional<double> frequencyMhz;
    std::optional<std::uint64_t> usedBytes;
    std::optional<std::uint64_t> totalBytes;
    std::optional<double> sendBitsPerSecond;
    std::optional<double> receiveBitsPerSecond;
    std::optional<double> temperatureCelsius;
};

/** Everything the Performance > CPU screen shows. */
struct CpuPerformance
{
    providers::CpuStaticInfo info;
    monitoring::CpuMetrics cpu;
    std::uint32_t processCount = 0;
    std::uint32_t threadCount = 0;
    std::optional<std::uint64_t> handleCount;
    std::chrono::system_clock::time_point bootTime;
    std::chrono::seconds uptime{};
    std::vector<monitoring::ProcessMetrics> topProcesses;
};

class IPerformanceService
{
public:
    virtual ~IPerformanceService() = default;

    /**
     * @brief List every monitored resource with its sparkline.
     * @return Resources in display order: CPU, memory, disks, network adapters, GPUs.
     * @throws std::runtime_error if no metrics snapshot is available.
     */
    [[nodiscard]] virtual std::vector<ResourceSummary> getResources() const = 0;

    /**
     * @brief Get the data of the CPU screen.
     * @return Current CPU performance.
     * @throws std::runtime_error if no metrics snapshot is available.
     */
    [[nodiscard]] virtual CpuPerformance getCpuPerformance() const = 0;
};

} // namespace systemlens::services::performance
