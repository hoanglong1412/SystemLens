/**
 * @file    CpuMetrics.h
 * @brief   Measured CPU values returned by CpuMonitorService (percentages and per-second rates),
 *          independent of the HTTP layer. Serialized by api/cpu/CpuJson.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::services::cpu {

/** Share of CPU time per part over one interval, in percent (0..100). */
struct CpuBreakdown
{
    double userPercent = 0;
    double kernelPercent = 0;
    double dpcPercent = 0;
    double interruptPercent = 0;
    double idlePercent = 100;
};

/** Scheduler events per second over one interval; empty when the platform has no counter. */
struct SchedulerRates
{
    std::optional<double> contextSwitchesPerSec;
    std::optional<double> interruptsPerSec;
    std::optional<double> systemCallsPerSec;
    std::optional<double> dpcsPerSec;
};

/** One row of the top CPU processes table. */
struct TopProcess
{
    std::uint32_t pid = 0;
    std::string name;
    double cpuPercent = 0;          ///< Share of the whole machine (all logical processors = 100).
    std::uint64_t memoryBytes = 0;
};

/** Values of one measured interval. */
struct CpuSnapshot
{
    double usagePercent = 0;
    std::vector<double> perCoreUsagePercent;
    std::optional<double> speedMhz;
    std::optional<double> temperatureCelsius;
    std::uint32_t processes = 0;
    std::uint32_t threads = 0;
    std::optional<std::uint64_t> handles;
    std::uint64_t uptimeSeconds = 0;
    std::int64_t bootTimeUnixSeconds = 0;
    CpuBreakdown breakdown;
    SchedulerRates scheduler;
    std::vector<TopProcess> topProcesses; ///< Highest CPU first, at most kTopProcessCount.
};

/** Last N intervals, oldest first; every array has the same length. */
struct CpuHistory
{
    std::vector<double> usagePercent;
    std::vector<std::optional<double>> speedMhz;
    std::vector<std::optional<double>> temperatureCelsius;
    std::vector<double> userPercent;
    std::vector<double> kernelPercent;
};

/** Response of CpuMonitorService::getMetrics(). */
struct CpuMetrics
{
    std::uint32_t sampleIntervalMs = 0;
    std::uint32_t historyLength = 0; ///< Capacity of the history (arrays may be shorter at startup).
    CpuSnapshot current;
    CpuHistory history;
};

} // namespace systemlens::services::cpu
