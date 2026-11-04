/**
 * @file    LinuxCpuProvider.cpp
 * @brief   Implementation of LinuxCpuProvider.
 */
#include "LinuxCpuProvider.h"

#include "LinuxProcFs.h"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>

namespace systemlens::plugin {

namespace {

namespace fs = std::filesystem;
using providers::cpu::CpuCounters;
using providers::cpu::CpuStaticInfo;
using providers::cpu::Virtualization;

constexpr double kKhzPerMhz = 1000.0;
constexpr double kMilliPerUnit = 1000.0;
constexpr const char* kCountersSource = "Source: /proc/stat, /proc/<pid>/stat";
/** Model shown when /proc/cpuinfo names none (e.g. some virtual machines). */
constexpr const char* kUnknownModel = "Unknown processor";
/**
 * hwmon drivers that report the CPU package / die temperature in temp1_input, by preference.
 * coretemp: Intel, k10temp / zenpower: AMD, cpu_thermal: Raspberry Pi and other ARM boards.
 */
constexpr std::array<const char*, 4> kCpuHwmonDrivers = {"coretemp", "k10temp", "zenpower", "cpu_thermal"};

/**
 * @brief Read a whole text file.
 * @param path File.
 * @return Content, or empty when the file cannot be read (missing, permission, process gone).
 */
std::optional<std::string> readFile(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        return std::nullopt;
    }
    std::ostringstream content;
    content << in.rdbuf();
    return content.str();
}

/**
 * @brief Read a file holding one unsigned integer (sysfs style).
 * @param path File.
 * @return Value, or empty when missing / malformed.
 */
std::optional<std::uint64_t> readUnsigned(const fs::path& path)
{
    const auto text = readFile(path);
    return text ? procfs::parseUnsigned(*text) : std::nullopt;
}

/**
 * @brief Read the first line of a file without the line break.
 * @param path File.
 * @return Line, or empty string when missing.
 */
std::string readFirstLine(const fs::path& path)
{
    const auto text = readFile(path);
    if (!text)
    {
        return {};
    }
    const auto end = text->find('\n');
    return end == std::string::npos ? *text : text->substr(0, end);
}

/**
 * @brief Directory of logical processor N in sysfs.
 * @param sysRoot sysfs root.
 * @param index Logical processor index.
 * @return e.g. /sys/devices/system/cpu/cpu3
 */
fs::path cpuDirectory(const fs::path& sysRoot, std::size_t index)
{
    return sysRoot / "devices/system/cpu" / ("cpu" + std::to_string(index));
}

/**
 * @brief Count packages and physical cores from sysfs topology.
 * @param sysRoot sysfs root.
 * @param logical Number of logical processors.
 * @param sockets Out: distinct physical_package_id (unchanged when unavailable).
 * @param cores Out: distinct (package, core_id) pairs (unchanged when unavailable).
 */
void readTopology(const fs::path& sysRoot, std::uint32_t logical, std::uint32_t& sockets, std::uint32_t& cores)
{
    std::set<std::uint64_t> packages;
    std::set<std::pair<std::uint64_t, std::uint64_t>> coreIds;
    for (std::uint32_t i = 0; i < logical; ++i)
    {
        const fs::path topology = cpuDirectory(sysRoot, i) / "topology";
        const auto package = readUnsigned(topology / "physical_package_id");
        const auto core = readUnsigned(topology / "core_id");
        if (!package || !core)
        {
            return;
        }
        packages.insert(*package);
        coreIds.insert({*package, *core});
    }
    if (!packages.empty())
    {
        sockets = static_cast<std::uint32_t>(packages.size());
        cores = static_cast<std::uint32_t>(coreIds.size());
    }
}

/**
 * @brief Sum the cache sizes per level from sysfs, counting each shared cache once.
 * @param sysRoot sysfs root.
 * @param logical Number of logical processors.
 * @param info Out: l1/l2/l3CacheBytes (left empty when no cache information exists).
 */
void readCaches(const fs::path& sysRoot, std::uint32_t logical, CpuStaticInfo& info)
{
    // A cache shared by several CPUs appears under each of them with the same shared_cpu_list.
    std::set<std::tuple<std::uint64_t, std::string, std::string>> seen;
    std::map<std::uint64_t, std::uint64_t> bytesPerLevel;
    for (std::uint32_t cpu = 0; cpu < logical; ++cpu)
    {
        const fs::path cacheDir = cpuDirectory(sysRoot, cpu) / "cache";
        std::error_code error;
        for (const auto& entry : fs::directory_iterator(cacheDir, error))
        {
            const auto name = entry.path().filename().string();
            if (name.rfind("index", 0) != 0)
            {
                continue;
            }
            const auto level = readUnsigned(entry.path() / "level");
            const auto size = procfs::parseCacheSizeBytes(readFirstLine(entry.path() / "size"));
            if (!level || !size)
            {
                continue;
            }
            const auto key = std::make_tuple(*level, readFirstLine(entry.path() / "type"),
                                             readFirstLine(entry.path() / "shared_cpu_list"));
            if (seen.insert(key).second)
            {
                bytesPerLevel[*level] += *size;
            }
        }
    }
    const auto level = [&bytesPerLevel](std::uint64_t n) -> std::optional<std::uint64_t> {
        const auto found = bytesPerLevel.find(n);
        return found == bytesPerLevel.end() ? std::nullopt : std::optional<std::uint64_t>(found->second);
    };
    info.l1CacheBytes = level(1);
    info.l2CacheBytes = level(2);
    info.l3CacheBytes = level(3);
}

/**
 * @brief Read a cpufreq value of logical processor 0 in MHz.
 * @param sysRoot sysfs root.
 * @param file e.g. "base_frequency", "cpuinfo_max_freq" (both in kHz).
 * @return MHz, or empty when the file does not exist (no cpufreq driver, VM, container).
 */
std::optional<double> readCpu0FrequencyMhz(const fs::path& sysRoot, const char* file)
{
    const auto khz = readUnsigned(cpuDirectory(sysRoot, 0) / "cpufreq" / file);
    return khz && *khz > 0 ? std::optional<double>(static_cast<double>(*khz) / kKhzPerMhz) : std::nullopt;
}

} // namespace

/**
 * @brief Create the provider.
 * @param procRoot procfs mount point.
 * @param sysRoot sysfs mount point.
 * @throws std::runtime_error when the clock tick rate or page size cannot be read.
 */
LinuxCpuProvider::LinuxCpuProvider(fs::path procRoot, fs::path sysRoot)
    : m_procRoot(std::move(procRoot))
    , m_sysRoot(std::move(sysRoot))
    , m_clockTicksPerSecond(sysconf(_SC_CLK_TCK))
    , m_pageSize(sysconf(_SC_PAGESIZE))
{
    if (m_clockTicksPerSecond <= 0 || m_pageSize <= 0)
    {
        throw std::runtime_error("sysconf(_SC_CLK_TCK / _SC_PAGESIZE) failed");
    }
}

/**
 * @brief Read model, topology, speeds, caches and virtualization support.
 * @return Static info.
 * @throws std::runtime_error when /proc/stat cannot be read.
 */
CpuStaticInfo LinuxCpuProvider::getStaticInfo() const
{
    const auto statText = readFile(m_procRoot / "stat");
    if (!statText)
    {
        throw std::runtime_error("cannot read " + (m_procRoot / "stat").string());
    }
    const auto stat = procfs::parseProcStat(*statText);
    const auto cpuinfo = procfs::parseCpuInfo(readFile(m_procRoot / "cpuinfo").value_or(""));

    CpuStaticInfo info;
    info.model = cpuinfo.modelName.empty() ? kUnknownModel : cpuinfo.modelName;
    // /proc/stat lists the online CPUs, which is also what readCounters() reports per core.
    info.logicalProcessors = static_cast<std::uint32_t>(std::max<std::size_t>(stat.perCore.size(), 1));
    info.sockets = cpuinfo.socketCount > 0 ? cpuinfo.socketCount : 1;
    info.cores = cpuinfo.coreCount > 0 ? cpuinfo.coreCount : info.logicalProcessors;
    readTopology(m_sysRoot, info.logicalProcessors, info.sockets, info.cores);

    info.baseSpeedMhz = readCpu0FrequencyMhz(m_sysRoot, "base_frequency");
    if (!info.baseSpeedMhz)
    {
        info.baseSpeedMhz = procfs::parseModelSpeedMhz(info.model);
    }
    if (!info.baseSpeedMhz)
    {
        info.baseSpeedMhz = cpuinfo.averageMhz;
    }
    info.maxSpeedMhz = readCpu0FrequencyMhz(m_sysRoot, "cpuinfo_max_freq");

    if (cpuinfo.hasFlags)
    {
        info.virtualization = cpuinfo.hasVirtualizationFlag ? Virtualization::Enabled : Virtualization::Disabled;
    }
    readCaches(m_sysRoot, info.logicalProcessors, info);
    info.countersSource = kCountersSource;
    return info;
}

/**
 * @brief Read all cumulative counters and gauges.
 * @return Counters (system calls are not exposed by Linux: always empty).
 * @throws std::runtime_error when /proc/stat cannot be read.
 */
CpuCounters LinuxCpuProvider::readCounters()
{
    const auto statText = readFile(m_procRoot / "stat");
    if (!statText)
    {
        throw std::runtime_error("cannot read " + (m_procRoot / "stat").string());
    }
    const auto stat = procfs::parseProcStat(*statText);

    CpuCounters counters;
    counters.total = stat.total;
    counters.perCore = stat.perCore;
    counters.contextSwitches = stat.contextSwitches;
    counters.interrupts = stat.interrupts;
    // Softirqs are the closest Linux equivalent of Windows DPCs (deferred interrupt work).
    counters.dpcs = stat.softirqs;
    counters.speedMhz = readCurrentSpeedMhz();
    counters.temperatureCelsius = readTemperatureCelsius();
    counters.handleCount = procfs::parseFileNrAllocated(readFile(m_procRoot / "sys/fs/file-nr").value_or(""));

    const auto uptime = procfs::parseUptimeSeconds(readFile(m_procRoot / "uptime").value_or(""));
    counters.uptimeSeconds = uptime ? static_cast<std::uint64_t>(*uptime) : 0;
    counters.bootTimeUnixSeconds = stat.bootTime.value_or(0);

    counters.processTimeUnitsPerSecond = static_cast<std::uint64_t>(m_clockTicksPerSecond);
    readProcesses(counters);
    return counters;
}

/**
 * @brief Mean current clock over the logical processors.
 * @return MHz from cpufreq, else from /proc/cpuinfo, else empty.
 */
std::optional<double> LinuxCpuProvider::readCurrentSpeedMhz() const
{
    double sumKhz = 0;
    std::size_t count = 0;
    for (std::size_t cpu = 0;; ++cpu)
    {
        const fs::path dir = cpuDirectory(m_sysRoot, cpu);
        std::error_code error;
        if (!fs::exists(dir, error))
        {
            break;
        }
        if (const auto khz = readUnsigned(dir / "cpufreq/scaling_cur_freq"))
        {
            sumKhz += static_cast<double>(*khz);
            ++count;
        }
    }
    if (count > 0)
    {
        return sumKhz / static_cast<double>(count) / kKhzPerMhz;
    }
    // No cpufreq (VMs, containers): /proc/cpuinfo still reports the clock seen by the kernel.
    return procfs::parseCpuInfo(readFile(m_procRoot / "cpuinfo").value_or("")).averageMhz;
}

/**
 * @brief Current CPU temperature.
 * @return Degrees Celsius, or empty when the machine exposes no CPU sensor.
 */
std::optional<double> LinuxCpuProvider::readTemperatureCelsius()
{
    if (!m_sensorSearched)
    {
        m_sensor = findTemperatureSensor();
        m_sensorSearched = true;
    }
    if (!m_sensor)
    {
        return std::nullopt;
    }
    const auto milliCelsius = readUnsigned(*m_sensor);
    return milliCelsius ? std::optional<double>(static_cast<double>(*milliCelsius) / kMilliPerUnit) : std::nullopt;
}

/**
 * @brief Find the file holding the CPU temperature.
 * @return hwmon temp1_input of a known CPU driver, else a CPU thermal zone temp, else empty.
 */
std::optional<fs::path> LinuxCpuProvider::findTemperatureSensor() const
{
    std::error_code error;
    std::map<std::string, fs::path> hwmonByDriver;
    for (const auto& entry : fs::directory_iterator(m_sysRoot / "class/hwmon", error))
    {
        const fs::path input = entry.path() / "temp1_input";
        if (fs::exists(input, error))
        {
            hwmonByDriver.emplace(readFirstLine(entry.path() / "name"), input);
        }
    }
    for (const char* driver : kCpuHwmonDrivers)
    {
        if (const auto found = hwmonByDriver.find(driver); found != hwmonByDriver.end())
        {
            return found->second;
        }
    }

    for (const auto& entry : fs::directory_iterator(m_sysRoot / "class/thermal", error))
    {
        const std::string type = readFirstLine(entry.path() / "type");
        if (type == "x86_pkg_temp" || type.find("cpu") != std::string::npos)
        {
            return entry.path() / "temp";
        }
    }
    return std::nullopt;
}

/**
 * @brief Scan /proc/<pid> for process counts and per-process counters.
 * @param counters Out: processCount, threadCount, processes.
 */
void LinuxCpuProvider::readProcesses(CpuCounters& counters) const
{
    std::error_code error;
    for (const auto& entry : fs::directory_iterator(m_procRoot, error))
    {
        const std::string name = entry.path().filename().string();
        if (!procfs::parseUnsigned(name))
        {
            continue;
        }
        // A process can exit between listing and reading: skip it silently.
        const auto statText = readFile(entry.path() / "stat");
        const auto stat = statText ? procfs::parseProcessStat(*statText) : std::nullopt;
        if (!stat)
        {
            continue;
        }
        const auto residentPages =
            procfs::parseStatmResidentPages(readFile(entry.path() / "statm").value_or("")).value_or(0);

        ++counters.processCount;
        counters.threadCount += stat->numThreads;
        counters.processes.push_back(providers::cpu::ProcessCounters{
            stat->pid,
            stat->comm,
            stat->utime + stat->stime,
            residentPages * static_cast<std::uint64_t>(m_pageSize),
        });
    }
}

} // namespace systemlens::plugin
