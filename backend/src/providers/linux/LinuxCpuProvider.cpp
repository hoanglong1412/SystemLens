/**
 * @file    LinuxCpuProvider.cpp
 * @brief   Implementation of LinuxCpuProvider.
 */
#include "providers/linux/LinuxCpuProvider.h"

#include "providers/ProviderError.h"
#include "providers/linux/ProcFs.h"

#include <sys/utsname.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <set>
#include <string_view>
#include <utility>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kProcStatFile = "/proc/stat";
constexpr const char* kCpuInfoFile = "/proc/cpuinfo";
constexpr const char* kCpuDir = "/sys/devices/system/cpu";
constexpr const char* kHwmonDir = "/sys/class/hwmon";
constexpr const char* kThermalDir = "/sys/class/thermal";
constexpr const char* kActivitySource = "/proc/stat (ctxt, intr, softirq)";

constexpr double kKhzPerMhz = 1000.0;
constexpr double kMhzPerGhz = 1000.0;
constexpr double kMillidegreesPerDegree = 1000.0;
constexpr std::uint64_t kBytesPerKib = 1024;
constexpr std::uint64_t kBytesPerMib = 1024 * 1024;

/** hwmon driver names that report the CPU package / die temperature. */
constexpr std::array<std::string_view, 5> kCpuSensorDrivers = {
    "coretemp", "k10temp", "zenpower", "cpu_thermal", "cpu-thermal"};
/** thermal_zone types used as a fallback when no hwmon driver matches. */
constexpr std::array<std::string_view, 3> kCpuThermalZoneTypes = {"x86_pkg_temp", "cpu-thermal", "cpu_thermal"};

/** Columns of the "cpu" line of /proc/stat, after the "cpu" label (see proc(5)). */
enum StatColumn : std::size_t
{
    kUser,
    kNice,
    kSystem,
    kIdle,
    kIoWait,
    kIrq,
    kSoftIrq,
    kSteal,
    kStatColumnCount,
};

/** What the per-processor blocks of /proc/cpuinfo say. */
struct CpuInfoSummary
{
    std::string model;
    std::uint32_t logicalProcessors = 0;
    std::set<std::string> sockets;                      ///< Distinct "physical id" values.
    std::set<std::pair<std::string, std::string>> cores; ///< Distinct (physical id, core id) pairs.
    bool hasVirtualizationFlag = false;                 ///< vmx (Intel) or svm (AMD).
    bool hasFlags = false;
};

/**
 * @brief Split a "key\t: value" line of /proc/cpuinfo.
 * @param line One line.
 * @return Trimmed key and value; both empty when the line has no colon.
 */
std::pair<std::string_view, std::string_view> splitKeyValue(std::string_view line)
{
    const auto colon = line.find(':');
    if (colon == std::string_view::npos)
    {
        return {};
    }
    return {procfs::trim(line.substr(0, colon)), procfs::trim(line.substr(colon + 1))};
}

/**
 * @brief Parse /proc/cpuinfo.
 * @return Summary; fields stay empty/zero for keys the architecture does not print (e.g. ARM has no "model name").
 * @throws ProviderError if the file is unreadable.
 */
CpuInfoSummary readCpuInfo()
{
    const std::string content = procfs::readFileOrThrow(kCpuInfoFile);
    CpuInfoSummary summary;
    std::string physicalId;
    for (const std::string_view line : procfs::splitLines(content))
    {
        const auto [key, value] = splitKeyValue(line);
        if (key == "processor")
        {
            ++summary.logicalProcessors;
        }
        else if (key == "model name" && summary.model.empty())
        {
            summary.model = value;
        }
        else if (key == "physical id")
        {
            physicalId = value;
            summary.sockets.insert(physicalId);
        }
        else if (key == "core id")
        {
            summary.cores.emplace(physicalId, value);
        }
        else if (key == "flags" && !summary.hasFlags)
        {
            summary.hasFlags = true;
            const auto flags = procfs::splitWhitespace(value);
            summary.hasVirtualizationFlag = std::ranges::any_of(
                flags, [](std::string_view flag) { return flag == "vmx" || flag == "svm"; });
        }
    }
    return summary;
}

/**
 * @brief Extract the nominal clock from a model string such as "... CPU @ 2.30GHz".
 * @param model Model name.
 * @return Clock in MHz, or std::nullopt when the name carries none.
 */
std::optional<double> parseSpeedFromModel(std::string_view model)
{
    const auto at = model.rfind('@');
    if (at == std::string_view::npos)
    {
        return std::nullopt;
    }
    std::string_view text = procfs::trim(model.substr(at + 1));
    const auto unit = text.find("GHz");
    if (unit == std::string_view::npos)
    {
        return std::nullopt;
    }
    const auto ghz = procfs::parseDouble(procfs::trim(text.substr(0, unit)));
    return ghz ? std::optional(*ghz * kMhzPerGhz) : std::nullopt;
}

/**
 * @brief Read a cpufreq value of the first processor.
 * @param fileName File under cpu0/cpufreq (values are in kHz).
 * @return Value in MHz, or std::nullopt when cpufreq is absent (typical in virtual machines).
 */
std::optional<double> readCpuFreqMhz(std::string_view fileName)
{
    const auto khz = procfs::readUnsigned(std::filesystem::path(kCpuDir) / "cpu0" / "cpufreq" / fileName);
    return khz ? std::optional(static_cast<double>(*khz) / kKhzPerMhz) : std::nullopt;
}

/**
 * @brief Parse a sysfs cache size such as "32K" or "8192K".
 * @param text Size with unit suffix.
 * @return Size in bytes, or std::nullopt when not understood.
 */
std::optional<std::uint64_t> parseCacheSize(std::string_view text)
{
    if (text.empty())
    {
        return std::nullopt;
    }
    const char unit = text.back();
    const auto number = procfs::parseUnsigned(text.substr(0, text.size() - 1));
    if (!number)
    {
        return std::nullopt;
    }
    switch (unit)
    {
    case 'K':
        return *number * kBytesPerKib;
    case 'M':
        return *number * kBytesPerMib;
    default:
        return std::nullopt;
    }
}

/**
 * @brief Sum the cache sizes per level over all processors.
 *
 * A cache shared by several logical processors appears under each of them, so caches are
 * de-duplicated by (level, type, shared_cpu_list).
 * @param info Receives l1CacheBytes / l2CacheBytes / l3CacheBytes for the levels that exist.
 */
void readCaches(CpuStaticInfo& info)
{
    std::set<std::string> seen;
    std::error_code error;
    for (const auto& cpu : std::filesystem::directory_iterator(kCpuDir, error))
    {
        const std::string name = cpu.path().filename().string();
        if (!name.starts_with("cpu") || !procfs::isAllDigits(std::string_view(name).substr(3)))
        {
            continue;
        }
        std::error_code cacheError;
        for (const auto& index : std::filesystem::directory_iterator(cpu.path() / "cache", cacheError))
        {
            const auto level = procfs::readUnsigned(index.path() / "level");
            const auto type = procfs::readTrimmed(index.path() / "type");
            const auto sharedWith = procfs::readTrimmed(index.path() / "shared_cpu_list");
            const auto size = parseCacheSize(procfs::readTrimmed(index.path() / "size").value_or(""));
            if (!level || !type || !sharedWith || !size)
            {
                continue;
            }
            if (!seen.insert(std::to_string(*level) + '|' + *type + '|' + *sharedWith).second)
            {
                continue;
            }
            std::optional<std::uint64_t>* total = *level == 1 ? &info.l1CacheBytes
                                                  : *level == 2 ? &info.l2CacheBytes
                                                  : *level == 3 ? &info.l3CacheBytes
                                                                : nullptr;
            if (total != nullptr)
            {
                *total = total->value_or(0) + *size;
            }
        }
    }
}

/**
 * @brief Convert the columns of a "cpu" / "cpuN" line of /proc/stat to CPU times.
 * @param columns Columns after the label (at least kStatColumnCount).
 * @param secondsPerTick Length of one clock tick.
 * @return Times split into user, kernel, softirq (dpc), irq (interrupt) and idle.
 */
CpuTimes toCpuTimes(const std::vector<std::string_view>& columns, double secondsPerTick)
{
    const auto seconds = [&](std::size_t column) {
        return CpuTimes::Duration(static_cast<double>(procfs::parseUnsigned(columns[column]).value_or(0))
                                  * secondsPerTick);
    };
    return CpuTimes{
        // Guest time is already included in user/nice by the kernel.
        .user = seconds(kUser) + seconds(kNice),
        .kernel = seconds(kSystem),
        .dpc = seconds(kSoftIrq),
        .interrupt = seconds(kIrq),
        // iowait and steal are time this system did not execute anything.
        .idle = seconds(kIdle) + seconds(kIoWait) + seconds(kSteal),
    };
}

/**
 * @brief Find the file that reports the CPU temperature.
 * @return Path of a millidegree file (hwmon temp1_input or thermal_zone temp), or std::nullopt.
 */
std::optional<std::filesystem::path> findTemperatureFile()
{
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(kHwmonDir, error))
    {
        const auto driver = procfs::readTrimmed(entry.path() / "name");
        const std::filesystem::path input = entry.path() / "temp1_input";
        if (driver && std::ranges::find(kCpuSensorDrivers, std::string_view(*driver)) != kCpuSensorDrivers.end()
            && std::filesystem::exists(input, error))
        {
            return input;
        }
    }
    for (const auto& entry : std::filesystem::directory_iterator(kThermalDir, error))
    {
        const auto type = procfs::readTrimmed(entry.path() / "type");
        const std::filesystem::path input = entry.path() / "temp";
        if (type && std::ranges::find(kCpuThermalZoneTypes, std::string_view(*type)) != kCpuThermalZoneTypes.end()
            && std::filesystem::exists(input, error))
        {
            return input;
        }
    }
    return std::nullopt;
}

/**
 * @brief Read the current clock averaged over all logical processors.
 * @return MHz from cpufreq scaling_cur_freq, falling back to the "cpu MHz" lines of /proc/cpuinfo; std::nullopt
 *         when neither exists.
 */
std::optional<double> readCurrentFrequencyMhz()
{
    double sumKhz = 0.0;
    std::uint32_t count = 0;
    std::error_code error;
    for (const auto& cpu : std::filesystem::directory_iterator(kCpuDir, error))
    {
        if (const auto khz = procfs::readUnsigned(cpu.path() / "cpufreq" / "scaling_cur_freq"))
        {
            sumKhz += static_cast<double>(*khz);
            ++count;
        }
    }
    if (count > 0)
    {
        return sumKhz / count / kKhzPerMhz;
    }

    const auto content = procfs::readFile(kCpuInfoFile);
    if (!content)
    {
        return std::nullopt;
    }
    double sumMhz = 0.0;
    for (const std::string_view line : procfs::splitLines(*content))
    {
        const auto [key, value] = splitKeyValue(line);
        if (key == "cpu MHz")
        {
            if (const auto mhz = procfs::parseDouble(value))
            {
                sumMhz += *mhz;
                ++count;
            }
        }
    }
    return count > 0 ? std::optional(sumMhz / count) : std::nullopt;
}

/**
 * @brief Read the first number after a "<key> " prefix in /proc/stat (e.g. "ctxt 12345", "intr 678 0 1 ...").
 * @param lines Lines of /proc/stat.
 * @param key   Line label followed by a space.
 * @return The number, or std::nullopt when the line is missing.
 */
std::optional<std::uint64_t> readStatCounter(const std::vector<std::string_view>& lines, std::string_view key)
{
    for (const std::string_view line : lines)
    {
        if (line.starts_with(key))
        {
            const auto fields = procfs::splitWhitespace(line.substr(key.size()));
            return fields.empty() ? std::nullopt : procfs::parseUnsigned(fields.front());
        }
    }
    return std::nullopt;
}

} // namespace

/**
 * @brief Cache the clock tick length and locate the temperature sensor.
 * @throws ProviderError if sysconf cannot report the tick rate.
 */
LinuxCpuProvider::LinuxCpuProvider()
    : m_temperatureFile(findTemperatureFile())
{
    const long ticksPerSecond = ::sysconf(_SC_CLK_TCK);
    if (ticksPerSecond <= 0)
    {
        throw ProviderError("sysconf(_SC_CLK_TCK) failed");
    }
    m_secondsPerTick = 1.0 / static_cast<double>(ticksPerSecond);
}

/**
 * @brief Read model, topology, nominal clock, virtualization flag and cache sizes.
 * @return Static information.
 * @throws ProviderError if /proc/cpuinfo is unreadable.
 */
CpuStaticInfo LinuxCpuProvider::readStaticInfo() const
{
    const CpuInfoSummary summary = readCpuInfo();

    CpuStaticInfo info;
    info.model = summary.model;
    if (info.model.empty())
    {
        // Architectures without "model name" (e.g. ARM): show at least the machine type.
        utsname kernel{};
        info.model = ::uname(&kernel) == 0 ? kernel.machine : "Unknown processor";
    }
    info.logicalProcessors = std::max<std::uint32_t>(summary.logicalProcessors, 1);
    // Without topology lines assume one socket whose cores are the logical processors.
    info.sockets = summary.sockets.empty() ? 1 : static_cast<std::uint32_t>(summary.sockets.size());
    info.cores = summary.cores.empty() ? info.logicalProcessors : static_cast<std::uint32_t>(summary.cores.size());

    info.maxSpeedMhz = readCpuFreqMhz("cpuinfo_max_freq");
    info.baseSpeedMhz = readCpuFreqMhz("base_frequency");
    if (!info.baseSpeedMhz)
    {
        info.baseSpeedMhz = parseSpeedFromModel(info.model);
    }
    if (!info.baseSpeedMhz)
    {
        info.baseSpeedMhz = info.maxSpeedMhz;
    }

    if (summary.hasFlags)
    {
        info.virtualization = summary.hasVirtualizationFlag ? VirtualizationState::Enabled
                                                            : VirtualizationState::Disabled;
    }
    readCaches(info);
    info.activitySource = kActivitySource;
    return info;
}

/**
 * @brief Read CPU times and event counters from /proc/stat, plus clock and temperature.
 * @return Current sample; systemCalls is always nullopt (Linux keeps no such counter).
 * @throws ProviderError if /proc/stat is unreadable or its "cpu" line is malformed.
 */
CpuSample LinuxCpuProvider::readSample()
{
    const std::string content = procfs::readFileOrThrow(kProcStatFile);
    const auto lines = procfs::splitLines(content);
    if (lines.empty() || !lines.front().starts_with("cpu "))
    {
        throw ProviderError("Unexpected /proc/stat format");
    }
    const auto columns = procfs::splitWhitespace(lines.front().substr(4));
    if (columns.size() < kStatColumnCount)
    {
        throw ProviderError("Too few columns in the cpu line of /proc/stat");
    }
    CpuSample sample;
    sample.times = toCpuTimes(columns, m_secondsPerTick);
    // The per-processor lines ("cpu0 ...", "cpu1 ...") follow the aggregate line.
    for (std::size_t index = 1; index < lines.size() && lines[index].starts_with("cpu"); ++index)
    {
        const auto coreColumns = procfs::splitWhitespace(lines[index]);
        if (coreColumns.size() > kStatColumnCount)
        {
            sample.perCore.push_back(toCpuTimes({coreColumns.begin() + 1, coreColumns.end()}, m_secondsPerTick));
        }
    }
    sample.activity = CpuActivityCounters{
        .contextSwitches = readStatCounter(lines, "ctxt "),
        .interrupts = readStatCounter(lines, "intr "),
        .systemCalls = std::nullopt,
        .dpcs = readStatCounter(lines, "softirq "),
    };
    sample.frequencyMhz = readCurrentFrequencyMhz();
    if (m_temperatureFile)
    {
        if (const auto millidegrees = procfs::readUnsigned(*m_temperatureFile))
        {
            sample.temperatureCelsius = static_cast<double>(*millidegrees) / kMillidegreesPerDegree;
        }
    }
    return sample;
}

} // namespace systemlens::providers::linux_os
