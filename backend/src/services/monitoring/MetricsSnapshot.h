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
};

/** Physical memory usage. */
struct MemoryMetrics
{
    std::uint64_t totalBytes = 0;
    std::uint64_t usedBytes = 0;
    double usedPercent = 0.0;
    std::vector<double> usedPercentHistory;
};

/** One physical disk. */
struct DiskMetrics
{
    std::string id;
    std::string name;
    providers::DiskKind kind = providers::DiskKind::Unknown;
    std::optional<double> activePercent;      ///< Share of the last interval with I/O in flight.
    std::vector<double> activePercentHistory; ///< 0 where not measurable.
};

/** One network adapter. */
struct NetworkMetrics
{
    std::string id;
    std::string name;
    providers::NetworkKind kind = providers::NetworkKind::Ethernet;
    double sendBitsPerSecond = 0.0;
    double receiveBitsPerSecond = 0.0;
    std::vector<double> throughputHistory; ///< send + receive, bits per second.
};

/** One graphics adapter. */
struct GpuMetrics
{
    std::string id;
    std::string name;
    std::string model;
    std::optional<double> utilizationPercent;
    std::optional<double> temperatureCelsius;
    std::vector<double> utilizationHistory; ///< 0 where not reported.
};

/** One process with its CPU share over the last interval. */
struct ProcessMetrics
{
    std::uint32_t pid = 0;
    std::string name;
    double cpuPercent = 0.0; ///< Share of the capacity of all logical processors.
    std::uint64_t memoryBytes = 0;
    bool isSelf = false; ///< The backend or the process that started it.
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

    std::vector<ProcessMetrics> topCpuProcesses; ///< Highest CPU first, at most kTopProcessCount.
    std::optional<ProcessMetrics> topMemoryProcess;
};

} // namespace systemlens::services::monitoring
