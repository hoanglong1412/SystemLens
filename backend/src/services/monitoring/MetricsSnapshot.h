/**
 * @file    MetricsSnapshot.h
 * @brief   Immutable result of one sampling tick: current values, rates and 60-second histories.
 *
 * Produced by MetricsCollector, consumed by the services that answer API requests.
 * Histories are oldest first, one value per sample interval.
 */
#pragma once

#include "providers/ICpuProvider.h"
#include "providers/IDiskProvider.h"
#include "providers/INetworkProvider.h"
#include "providers/ISystemInfoProvider.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::services::monitoring {

/** CPU load, clock, temperature, time split and scheduler rates over the last interval. */
struct CpuMetrics
{
    double usagePercent = 0.0; ///< 100 - idlePercent.
    std::optional<double> frequencyMhz;
    std::optional<double> frequencyPercent; ///< frequencyMhz relative to the highest clock known.
    std::optional<double> temperatureCelsius;

    double userPercent = 0.0;
    double kernelPercent = 0.0;
    double dpcPercent = 0.0;
    double interruptPercent = 0.0;
    double idlePercent = 100.0;

    std::optional<double> contextSwitchesPerSecond;
    std::optional<double> interruptsPerSecond;
    std::optional<double> systemCallsPerSecond;
    std::optional<double> dpcsPerSecond;

    std::vector<double> usageHistory;
    std::vector<double> userHistory;
    std::vector<double> kernelHistory; ///< kernel + dpc + interrupt.
    std::vector<std::optional<double>> frequencyPercentHistory;
    std::vector<std::optional<double>> temperatureHistory;

    std::vector<double> coreUsagePercent;              ///< One entry per logical processor.
    std::vector<std::vector<double>> coreUsageHistory; ///< Same order; each oldest first.
};

/** Physical memory usage. */
struct MemoryMetrics
{
    std::uint64_t totalBytes = 0;
    std::uint64_t usedBytes = 0;
    std::uint64_t availableBytes = 0;
    std::optional<std::uint64_t> cachedBytes;
    std::optional<std::uint64_t> committedBytes;
    std::optional<std::uint64_t> commitLimitBytes;
    double usedPercent = 0.0;
    std::vector<double> usedPercentHistory;
};

/** One physical disk. */
struct DiskMetrics
{
    std::string id;
    std::string name;
    std::string model;
    providers::DiskKind kind = providers::DiskKind::Unknown;
    std::optional<std::uint64_t> capacityBytes;
    std::optional<double> activePercent;      ///< Share of the last interval with I/O in flight.
    std::optional<double> readBytesPerSecond;
    std::optional<double> writeBytesPerSecond;
    std::vector<double> activePercentHistory; ///< 0 where not measurable.
    std::vector<double> readHistory;          ///< bytes per second, 0 where not measurable.
    std::vector<double> writeHistory;
};

/** One network adapter. */
struct NetworkMetrics
{
    std::string id;
    std::string name;
    std::string description;
    std::string macAddress;
    providers::NetworkKind kind = providers::NetworkKind::Ethernet;
    std::optional<double> linkSpeedBitsPerSecond;
    double sendBitsPerSecond = 0.0;
    double receiveBitsPerSecond = 0.0;
    std::vector<double> throughputHistory; ///< send + receive, bits per second.
    std::vector<double> sendHistory;
    std::vector<double> receiveHistory;
};

/** One graphics adapter. */
struct GpuMetrics
{
    std::string id;
    std::string name;
    std::string model;
    std::optional<double> utilizationPercent;
    std::optional<double> temperatureCelsius;
    std::optional<std::uint64_t> memoryTotalBytes;
    std::optional<std::uint64_t> memoryUsedBytes;
    std::vector<double> utilizationHistory; ///< 0 where not reported.
};

/** One process with its CPU share over the last interval. */
struct ProcessMetrics
{
    std::uint32_t pid = 0;
    std::uint32_t parentPid = 0;
    std::string name;
    std::string userName;
    double cpuPercent = 0.0; ///< Share of the capacity of all logical processors.
    double cpuTimeSeconds = 0.0;
    std::uint64_t memoryBytes = 0;
    std::uint32_t threadCount = 0;
    std::string status;
    std::string commandLine;
    bool isSelf = false; ///< The backend or the process that started it.
};

/** Usage of one application name accumulated since the collector started. */
struct AppUsage
{
    std::string name;
    double cpuTimeSeconds = 0.0;
    std::uint64_t peakMemoryBytes = 0;
    std::uint64_t memoryBytes = 0;
    std::uint32_t runningInstances = 0;
};

/** Everything the API serves, as of one sampling tick. */
struct MetricsSnapshot
{
    std::chrono::system_clock::time_point takenAt;
    std::chrono::milliseconds sampleInterval{};

    providers::SystemIdentity identity;
    std::chrono::system_clock::time_point bootTime;
    providers::SystemCounts counts;

    providers::CpuStaticInfo cpuInfo;
    CpuMetrics cpu;
    MemoryMetrics memory;
    std::vector<DiskMetrics> disks;
    std::vector<NetworkMetrics> networks;
    std::vector<GpuMetrics> gpus;

    std::vector<ProcessMetrics> processes;       ///< Every process, highest CPU first.
    std::vector<ProcessMetrics> topCpuProcesses; ///< First kTopProcessCount of processes.
    std::optional<ProcessMetrics> topMemoryProcess;

    std::chrono::system_clock::time_point appHistorySince;
    std::vector<AppUsage> appHistory; ///< Highest accumulated CPU time first, at most kAppHistoryLimit.
};

} // namespace systemlens::services::monitoring
