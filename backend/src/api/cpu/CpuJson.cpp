/**
 * @file    CpuJson.cpp
 * @brief   Implementation of the CPU JSON serializers.
 */
#include "api/cpu/CpuJson.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <optional>
#include <vector>

namespace systemlens::api::cpu {

namespace {

using providers::cpu::Virtualization;

/** Decimals kept for measured values: finer digits are noise and only grow the payload. */
constexpr double kRoundingFactor = 100.0;

/**
 * @brief Round a measured value to two decimals.
 * @param value Value.
 * @return Rounded value.
 */
double round2(double value)
{
    return std::round(value * kRoundingFactor) / kRoundingFactor;
}

/**
 * @brief Optional double → rounded number or null.
 * @param value Optional value.
 * @return JSON number or null.
 */
nlohmann::json optionalNumber(const std::optional<double>& value)
{
    return value ? nlohmann::json(round2(*value)) : nlohmann::json(nullptr);
}

/**
 * @brief Optional integer → number or null.
 * @param value Optional value.
 * @return JSON number or null.
 */
nlohmann::json optionalInteger(const std::optional<std::uint64_t>& value)
{
    return value ? nlohmann::json(*value) : nlohmann::json(nullptr);
}

/**
 * @brief Array of doubles, rounded.
 * @param values Values.
 * @return JSON array.
 */
nlohmann::json numberArray(const std::vector<double>& values)
{
    nlohmann::json array = nlohmann::json::array();
    for (const double v : values)
    {
        array.push_back(round2(v));
    }
    return array;
}

/**
 * @brief Array of optional doubles, rounded, null for missing samples.
 * @param values Values.
 * @return JSON array.
 */
nlohmann::json optionalNumberArray(const std::vector<std::optional<double>>& values)
{
    nlohmann::json array = nlohmann::json::array();
    for (const auto& v : values)
    {
        array.push_back(optionalNumber(v));
    }
    return array;
}

/**
 * @brief Virtualization state → contract string.
 * @param value State.
 * @return "enabled", "disabled" or "unknown".
 */
const char* toString(Virtualization value)
{
    switch (value)
    {
    case Virtualization::Enabled:
        return "enabled";
    case Virtualization::Disabled:
        return "disabled";
    case Virtualization::Unknown:
        break;
    }
    return "unknown";
}

/**
 * @brief Serialize one interval.
 * @param s Snapshot.
 * @return JSON object (schema CpuSnapshot).
 */
nlohmann::json snapshotToJson(const services::cpu::CpuSnapshot& s)
{
    nlohmann::json processes = nlohmann::json::array();
    for (const auto& p : s.topProcesses)
    {
        processes.push_back({
            {"pid", p.pid},
            {"name", p.name},
            {"cpuPercent", round2(p.cpuPercent)},
            {"memoryBytes", p.memoryBytes},
        });
    }

    return {
        {"usagePercent", round2(s.usagePercent)},
        {"perCoreUsagePercent", numberArray(s.perCoreUsagePercent)},
        {"speedMhz", optionalNumber(s.speedMhz)},
        {"temperatureCelsius", optionalNumber(s.temperatureCelsius)},
        {"processes", s.processes},
        {"threads", s.threads},
        {"handles", optionalInteger(s.handles)},
        {"uptimeSeconds", s.uptimeSeconds},
        {"bootTime", toIso8601Utc(s.bootTimeUnixSeconds)},
        {"breakdown",
         {
             {"userPercent", round2(s.breakdown.userPercent)},
             {"kernelPercent", round2(s.breakdown.kernelPercent)},
             {"dpcPercent", round2(s.breakdown.dpcPercent)},
             {"interruptPercent", round2(s.breakdown.interruptPercent)},
             {"idlePercent", round2(s.breakdown.idlePercent)},
         }},
        {"scheduler",
         {
             {"contextSwitchesPerSec", optionalNumber(s.scheduler.contextSwitchesPerSec)},
             {"interruptsPerSec", optionalNumber(s.scheduler.interruptsPerSec)},
             {"systemCallsPerSec", optionalNumber(s.scheduler.systemCallsPerSec)},
             {"dpcsPerSec", optionalNumber(s.scheduler.dpcsPerSec)},
         }},
        {"topProcesses", processes},
    };
}

} // namespace

/**
 * @brief Serialize the static processor facts.
 * @param info Static info.
 * @return JSON object (schema CpuInfo).
 */
nlohmann::json toJson(const providers::cpu::CpuStaticInfo& info)
{
    return {
        {"model", info.model},
        {"sockets", info.sockets},
        {"cores", info.cores},
        {"logicalProcessors", info.logicalProcessors},
        {"baseSpeedMhz", optionalNumber(info.baseSpeedMhz)},
        {"maxSpeedMhz", optionalNumber(info.maxSpeedMhz)},
        {"virtualization", toString(info.virtualization)},
        {"cache",
         {
             {"l1Bytes", optionalInteger(info.l1CacheBytes)},
             {"l2Bytes", optionalInteger(info.l2CacheBytes)},
             {"l3Bytes", optionalInteger(info.l3CacheBytes)},
         }},
        {"countersSource", info.countersSource},
    };
}

/**
 * @brief Serialize the live metrics.
 * @param metrics Metrics with history.
 * @return JSON object (schema CpuMetrics).
 */
nlohmann::json toJson(const services::cpu::CpuMetrics& metrics)
{
    return {
        {"sampleIntervalMs", metrics.sampleIntervalMs},
        {"historyLength", metrics.historyLength},
        {"current", snapshotToJson(metrics.current)},
        {"history",
         {
             {"usagePercent", numberArray(metrics.history.usagePercent)},
             {"speedMhz", optionalNumberArray(metrics.history.speedMhz)},
             {"temperatureCelsius", optionalNumberArray(metrics.history.temperatureCelsius)},
             {"userPercent", numberArray(metrics.history.userPercent)},
             {"kernelPercent", numberArray(metrics.history.kernelPercent)},
         }},
    };
}

/**
 * @brief Format a Unix time as an RFC 3339 UTC timestamp.
 * @param unixSeconds Seconds since 1970-01-01T00:00:00Z.
 * @return e.g. "2026-09-14T08:12:41Z".
 */
std::string toIso8601Utc(std::int64_t unixSeconds)
{
    // std::chrono calendar types instead of gmtime_r / gmtime_s, which differ between platforms.
    constexpr std::int64_t kSecondsPerDay = 86400;
    constexpr std::int64_t kSecondsPerHour = 3600;
    constexpr std::int64_t kSecondsPerMinute = 60;
    std::int64_t days = unixSeconds / kSecondsPerDay;
    std::int64_t secondsOfDay = unixSeconds % kSecondsPerDay;
    if (secondsOfDay < 0)
    {
        secondsOfDay += kSecondsPerDay;
        --days;
    }
    const std::chrono::year_month_day date{std::chrono::sys_days{std::chrono::days{days}}};

    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02u-%02uT%02lld:%02lld:%02lldZ", static_cast<int>(date.year()),
                  static_cast<unsigned>(date.month()), static_cast<unsigned>(date.day()),
                  static_cast<long long>(secondsOfDay / kSecondsPerHour),
                  static_cast<long long>(secondsOfDay / kSecondsPerMinute % kSecondsPerMinute),
                  static_cast<long long>(secondsOfDay % kSecondsPerMinute));
    return buffer;
}

} // namespace systemlens::api::cpu
