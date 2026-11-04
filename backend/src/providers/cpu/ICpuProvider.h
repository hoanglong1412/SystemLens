/**
 * @file    ICpuProvider.h
 * @brief   Provider interface implemented by the platform plugin: static processor facts and
 *          cumulative CPU counters behind GET /api/v1/cpu/info and GET /api/v1/cpu/metrics.
 *
 * The provider only reads the OS. Deltas, rates, percentages and history are computed on the host
 * (services/cpu/CpuMetricsCalculator), so every platform shares and tests the same arithmetic.
 *
 * Part of the plugin ABI (see PluginAbi.h): any change to this file requires bumping kPluginAbiVersion.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::providers::cpu {

/** Whether hardware virtualization is usable. */
enum class Virtualization
{
    Unknown,
    Enabled,
    Disabled,
};

/** Facts that do not change while the machine runs. */
struct CpuStaticInfo
{
    std::string model;                        ///< Marketing name, e.g. "Intel(R) Core(TM) i5-8300H CPU @ 2.30GHz".
    std::uint32_t sockets = 1;                ///< Physical packages.
    std::uint32_t cores = 1;                  ///< Physical cores, all packages.
    std::uint32_t logicalProcessors = 1;      ///< Hardware threads, all packages.
    std::optional<double> baseSpeedMhz;       ///< Nominal clock.
    std::optional<double> maxSpeedMhz;        ///< Highest clock the OS reports (turbo), if known.
    Virtualization virtualization = Virtualization::Unknown;
    std::optional<std::uint64_t> l1CacheBytes; ///< Sum over all L1 caches (data + instruction).
    std::optional<std::uint64_t> l2CacheBytes; ///< Sum over all L2 caches.
    std::optional<std::uint64_t> l3CacheBytes; ///< Sum over all L3 caches.
    std::string countersSource;               ///< Where the scheduler counters come from (shown in the UI).
};

/**
 * Cumulative CPU time split in disjoint parts. The unit is provider-specific but identical for all
 * fields and all reads of one provider (only ratios of deltas are used).
 */
struct CpuTimes
{
    std::uint64_t user = 0;      ///< Applications (Linux: user + nice).
    std::uint64_t kernel = 0;    ///< OS code, excluding dpc, interrupt and idle.
    std::uint64_t dpc = 0;       ///< Deferred procedure calls (Linux: softirq).
    std::uint64_t interrupt = 0; ///< Hardware interrupts (Linux: irq).
    std::uint64_t idle = 0;      ///< Nothing to run (Linux: idle + iowait).
};

/** Cumulative counters of one process. */
struct ProcessCounters
{
    std::uint32_t pid = 0;
    std::string name;             ///< Executable / command name.
    std::uint64_t cpuTime = 0;    ///< User + kernel time, in CpuCounters::processTimeUnitsPerSecond units.
    std::uint64_t memoryBytes = 0; ///< Private working set (Windows) / resident set size (Linux).
};

/** One read of the OS counters. Cumulative fields only make sense as a delta between two reads. */
struct CpuCounters
{
    CpuTimes total;                ///< All logical processors together.
    std::vector<CpuTimes> perCore; ///< One entry per logical processor, OS order.

    std::optional<std::uint64_t> contextSwitches; ///< Cumulative; may be a wrapping 32-bit counter.
    std::optional<std::uint64_t> interrupts;      ///< Cumulative; may be a wrapping 32-bit counter.
    std::optional<std::uint64_t> systemCalls;     ///< Cumulative; may be a wrapping 32-bit counter.
    std::optional<std::uint64_t> dpcs;            ///< Cumulative; may be a wrapping 32-bit counter.

    std::optional<double> speedMhz;           ///< Current average clock.
    std::optional<double> temperatureCelsius; ///< Current package / hottest zone temperature.

    std::uint32_t processCount = 0;
    std::uint32_t threadCount = 0;
    std::optional<std::uint64_t> handleCount; ///< Windows handles / Linux allocated file handles.

    std::uint64_t uptimeSeconds = 0;
    std::int64_t bootTimeUnixSeconds = 0;

    std::vector<ProcessCounters> processes;
    std::uint64_t processTimeUnitsPerSecond = 1; ///< Unit of ProcessCounters::cpuTime.
};

class ICpuProvider
{
public:
    virtual ~ICpuProvider() = default;

    /**
     * @brief Read the facts that do not change while the machine runs.
     * @return Static processor information; unknown values are empty optionals.
     * @throws std::exception when the OS cannot be queried at all.
     */
    virtual CpuStaticInfo getStaticInfo() const = 0;

    /**
     * @brief Read the current cumulative counters.
     *
     * Non-const: a provider may keep OS query handles between reads (e.g. a PDH query).
     * Called from a single thread (the sampling thread of CpuMonitorService).
     * @return Counters; values the platform cannot report are empty optionals.
     * @throws std::exception when the OS cannot be queried.
     */
    virtual CpuCounters readCounters() = 0;
};

} // namespace systemlens::providers::cpu
