/**
 * @file    CpuMetricsCalculator.cpp
 * @brief   Implementation of the CPU delta / rate / percentage computations.
 */
#include "services/cpu/CpuMetricsCalculator.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>

namespace systemlens::services::cpu {

namespace {

using providers::cpu::CpuCounters;
using providers::cpu::CpuTimes;

constexpr double kFullPercent = 100.0;
/** 2^32: a 32-bit counter that wraps restarts from 0 after UINT32_MAX. */
constexpr std::uint64_t kUint32Range = std::uint64_t{1} << 32;

/**
 * @brief Delta of a monotonic time counter; a decrease (counter reset) counts as 0.
 * @param previous Older value.
 * @param current Newer value.
 * @return current - previous, or 0 when current < previous.
 */
std::uint64_t timeDelta(std::uint64_t previous, std::uint64_t current)
{
    return current >= previous ? current - previous : 0;
}

/**
 * @brief Clamp a percentage to [0, 100] (rounding of OS counters can step slightly outside).
 * @param value Percentage.
 * @return Clamped value.
 */
double clampPercent(double value)
{
    return std::clamp(value, 0.0, kFullPercent);
}

/**
 * @brief Per-second rate of an optional cumulative event counter.
 * @param previous Older value (empty when the platform has no counter).
 * @param current Newer value.
 * @param elapsedSeconds Interval length; must be > 0.
 * @return Events per second, or empty when a value is missing or the counter was reset.
 */
std::optional<double> rate(const std::optional<std::uint64_t>& previous, const std::optional<std::uint64_t>& current,
                           double elapsedSeconds)
{
    if (!previous || !current || elapsedSeconds <= 0)
    {
        return std::nullopt;
    }
    const auto delta = counterDelta(*previous, *current);
    if (!delta)
    {
        return std::nullopt;
    }
    return static_cast<double>(*delta) / elapsedSeconds;
}

/**
 * @brief Top processes by CPU usage in the interval.
 * @param previous Older read.
 * @param current Newer read.
 * @param elapsedSeconds Interval length.
 * @param logicalProcessors Divisor that makes the whole machine 100 %.
 * @return At most kTopProcessCount rows, highest CPU first, ties by ascending pid.
 */
std::vector<TopProcess> topProcesses(const CpuCounters& previous, const CpuCounters& current, double elapsedSeconds,
                                     std::uint32_t logicalProcessors)
{
    // A pid can be reused by a new process between two reads: matching on (pid, name) avoids
    // attributing the old process' CPU time to the new one (it gets 0 % for its first interval).
    std::map<std::pair<std::uint32_t, std::string>, std::uint64_t> previousTimes;
    for (const auto& process : previous.processes)
    {
        previousTimes[{process.pid, process.name}] = process.cpuTime;
    }

    const double capacity = elapsedSeconds * static_cast<double>(current.processTimeUnitsPerSecond) *
                            static_cast<double>(std::max<std::uint32_t>(logicalProcessors, 1));

    std::vector<TopProcess> rows;
    rows.reserve(current.processes.size());
    for (const auto& process : current.processes)
    {
        double cpuPercent = 0;
        const auto found = previousTimes.find({process.pid, process.name});
        if (found != previousTimes.end() && capacity > 0)
        {
            cpuPercent = clampPercent(static_cast<double>(timeDelta(found->second, process.cpuTime)) / capacity *
                                      kFullPercent);
        }
        rows.push_back(TopProcess{process.pid, process.name, cpuPercent, process.memoryBytes});
    }

    const auto byCpuThenPid = [](const TopProcess& a, const TopProcess& b) {
        return a.cpuPercent != b.cpuPercent ? a.cpuPercent > b.cpuPercent : a.pid < b.pid;
    };
    const std::size_t count = std::min(kTopProcessCount, rows.size());
    std::partial_sort(rows.begin(), rows.begin() + static_cast<std::ptrdiff_t>(count), rows.end(), byCpuThenPid);
    rows.resize(count);
    return rows;
}

} // namespace

/**
 * @brief Delta of a cumulative event counter, tolerant of 32-bit wrap-around.
 * @param previous Older value.
 * @param current Newer value.
 * @return current - previous; across a wrap when previous fits in 32 bits and current < previous;
 *         empty when the counter went backwards otherwise (reset).
 */
std::optional<std::uint64_t> counterDelta(std::uint64_t previous, std::uint64_t current)
{
    if (current >= previous)
    {
        return current - previous;
    }
    // Windows exposes several counters as 32-bit values (PDH PERF_COUNTER_COUNTER) that wrap.
    if (previous < kUint32Range)
    {
        return current + kUint32Range - previous;
    }
    return std::nullopt;
}

/**
 * @brief Share of each CPU time part between two reads.
 * @param previous Older times.
 * @param current Newer times.
 * @return Percentages summing to 100; 100 % idle when no time elapsed.
 */
CpuBreakdown computeBreakdown(const CpuTimes& previous, const CpuTimes& current)
{
    const double user = static_cast<double>(timeDelta(previous.user, current.user));
    const double kernel = static_cast<double>(timeDelta(previous.kernel, current.kernel));
    const double dpc = static_cast<double>(timeDelta(previous.dpc, current.dpc));
    const double interrupt = static_cast<double>(timeDelta(previous.interrupt, current.interrupt));
    const double idle = static_cast<double>(timeDelta(previous.idle, current.idle));
    const double total = user + kernel + dpc + interrupt + idle;

    CpuBreakdown breakdown;
    if (total <= 0)
    {
        return breakdown;
    }
    breakdown.userPercent = user / total * kFullPercent;
    breakdown.kernelPercent = kernel / total * kFullPercent;
    breakdown.dpcPercent = dpc / total * kFullPercent;
    breakdown.interruptPercent = interrupt / total * kFullPercent;
    breakdown.idlePercent = idle / total * kFullPercent;
    return breakdown;
}

/**
 * @brief Compute the metrics of the interval between two reads.
 * @param previous Older read.
 * @param current Newer read (instantaneous values are taken from it).
 * @param elapsedSeconds Wall-clock length of the interval; rates are empty when <= 0.
 * @param logicalProcessors Logical processor count used to normalize process CPU usage.
 * @return Snapshot of the interval.
 */
CpuSnapshot computeSnapshot(const CpuCounters& previous, const CpuCounters& current, double elapsedSeconds,
                            std::uint32_t logicalProcessors)
{
    CpuSnapshot snapshot;
    snapshot.breakdown = computeBreakdown(previous.total, current.total);
    snapshot.usagePercent = clampPercent(kFullPercent - snapshot.breakdown.idlePercent);

    snapshot.perCoreUsagePercent.reserve(current.perCore.size());
    for (std::size_t i = 0; i < current.perCore.size(); ++i)
    {
        // A core that appeared since the previous read (CPU hot-plug) has no baseline yet.
        const double usage = i < previous.perCore.size()
                                 ? kFullPercent - computeBreakdown(previous.perCore[i], current.perCore[i]).idlePercent
                                 : 0.0;
        snapshot.perCoreUsagePercent.push_back(clampPercent(usage));
    }

    snapshot.speedMhz = current.speedMhz;
    snapshot.temperatureCelsius = current.temperatureCelsius;
    snapshot.processes = current.processCount;
    snapshot.threads = current.threadCount;
    snapshot.handles = current.handleCount;
    snapshot.uptimeSeconds = current.uptimeSeconds;
    snapshot.bootTimeUnixSeconds = current.bootTimeUnixSeconds;

    snapshot.scheduler.contextSwitchesPerSec = rate(previous.contextSwitches, current.contextSwitches, elapsedSeconds);
    snapshot.scheduler.interruptsPerSec = rate(previous.interrupts, current.interrupts, elapsedSeconds);
    snapshot.scheduler.systemCallsPerSec = rate(previous.systemCalls, current.systemCalls, elapsedSeconds);
    snapshot.scheduler.dpcsPerSec = rate(previous.dpcs, current.dpcs, elapsedSeconds);

    snapshot.topProcesses = topProcesses(previous, current, elapsedSeconds, logicalProcessors);
    return snapshot;
}

} // namespace systemlens::services::cpu
