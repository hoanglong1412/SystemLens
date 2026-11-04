/**
 * @file    WindowsCpuProvider.cpp
 * @brief   Implementation of WindowsCpuProvider.
 */
#include "WindowsCpuProvider.h"

#include "WideString.h"

#include <windows.h>
#include <winternl.h>
#include <pdh.h>
#include <pdhmsg.h>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <ctime>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace systemlens::plugin {

namespace {

using providers::cpu::CpuCounters;
using providers::cpu::CpuStaticInfo;
using providers::cpu::CpuTimes;
using providers::cpu::Virtualization;

/** NTSTATUS returned when the buffer given to NtQuerySystemInformation is too small. */
constexpr NTSTATUS kStatusInfoLengthMismatch = static_cast<NTSTATUS>(0xC0000004L);
/** SYSTEM_INFORMATION_CLASS values (documented in winternl.h; repeated for SDKs that omit them). */
constexpr auto kSystemProcessInformation = static_cast<SYSTEM_INFORMATION_CLASS>(5);
constexpr auto kSystemProcessorPerformanceInformation = static_cast<SYSTEM_INFORMATION_CLASS>(8);
/** IsProcessorFeaturePresent: virtualization enabled in firmware (Windows 8+). */
constexpr DWORD kPfVirtFirmwareEnabled = 21;
/** NtQuerySystemInformation process times and GetSystemTimes use 100 ns units. */
constexpr std::uint64_t kHundredNanosecondsPerSecond = 10'000'000;
constexpr std::uint64_t kMillisecondsPerSecond = 1000;
constexpr double kKelvinToCelsius = 273.15;
constexpr double kFullPercent = 100.0;
/** Initial process snapshot buffer; grown on STATUS_INFO_LENGTH_MISMATCH. */
constexpr ULONG kInitialProcessBufferBytes = 512 * 1024;
/** Extra room for processes started between the size query and the real query. */
constexpr ULONG kProcessBufferSlackBytes = 64 * 1024;
/** Processor group 0 holds at most 64 logical processors. */
constexpr std::size_t kMaxGroupProcessors = 64;
constexpr int kMaxNtQueryAttempts = 5;

constexpr const wchar_t* kProcessorRegistryKey = L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
constexpr const wchar_t* kCountersSourceText = L"Source: PDH \\System, \\Processor(_Total)";

/**
 * Layout of SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION with the fields that winternl.h names
 * Reserved1[0] (DpcTime), Reserved1[1] (InterruptTime) and Reserved2 (InterruptCount).
 */
struct ProcessorPerformance
{
    LARGE_INTEGER idleTime;
    LARGE_INTEGER kernelTime; ///< Includes idle, DPC and interrupt time.
    LARGE_INTEGER userTime;
    LARGE_INTEGER dpcTime;
    LARGE_INTEGER interruptTime;
    ULONG interruptCount;
};

/**
 * Layout of SYSTEM_PROCESS_INFORMATION with the fields that winternl.h hides in Reserved1
 * (WorkingSetPrivateSize, ..., UserTime, KernelTime). Only the fields read here are declared.
 */
struct ProcessEntry
{
    ULONG nextEntryOffset;
    ULONG numberOfThreads;
    LARGE_INTEGER workingSetPrivateSize;
    ULONG hardFaultCount;
    ULONG numberOfThreadsHighWatermark;
    ULONGLONG cycleTime;
    LARGE_INTEGER createTime;
    LARGE_INTEGER userTime;
    LARGE_INTEGER kernelTime;
    UNICODE_STRING imageName;
    LONG basePriority;
    HANDLE uniqueProcessId;
    HANDLE inheritedFromUniqueProcessId;
    ULONG handleCount;
};

#if defined(_MSC_VER)
// The Windows SDK declares the public fields; the hidden ones must line up with them.
static_assert(offsetof(ProcessEntry, imageName) == offsetof(SYSTEM_PROCESS_INFORMATION, ImageName));
static_assert(offsetof(ProcessEntry, uniqueProcessId) == offsetof(SYSTEM_PROCESS_INFORMATION, UniqueProcessId));
static_assert(offsetof(ProcessEntry, handleCount) == offsetof(SYSTEM_PROCESS_INFORMATION, HandleCount));
#endif

/**
 * @brief LARGE_INTEGER → unsigned (cumulative OS times are never negative).
 * @param value Value.
 * @return Value, 0 when negative.
 */
std::uint64_t toUnsigned(const LARGE_INTEGER& value)
{
    return value.QuadPart > 0 ? static_cast<std::uint64_t>(value.QuadPart) : 0;
}

/**
 * @brief Read a REG_SZ value of HKLM.
 * @param key Subkey.
 * @param name Value name.
 * @return Value, or empty when missing.
 */
std::wstring readRegistryString(const wchar_t* key, const wchar_t* name)
{
    DWORD bytes = 0;
    if (RegGetValueW(HKEY_LOCAL_MACHINE, key, name, RRF_RT_REG_SZ, nullptr, nullptr, &bytes) != ERROR_SUCCESS ||
        bytes == 0)
    {
        return {};
    }
    std::wstring value(bytes / sizeof(wchar_t), L'\0');
    if (RegGetValueW(HKEY_LOCAL_MACHINE, key, name, RRF_RT_REG_SZ, nullptr, value.data(), &bytes) != ERROR_SUCCESS)
    {
        return {};
    }
    value.resize(wcsnlen(value.c_str(), value.size()));
    return value;
}

/**
 * @brief Read a REG_DWORD value of HKLM.
 * @param key Subkey.
 * @param name Value name.
 * @return Value, or empty when missing.
 */
std::optional<DWORD> readRegistryDword(const wchar_t* key, const wchar_t* name)
{
    DWORD value = 0;
    DWORD bytes = sizeof(value);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, key, name, RRF_RT_REG_DWORD, nullptr, &value, &bytes) != ERROR_SUCCESS)
    {
        return std::nullopt;
    }
    return value;
}

/**
 * @brief Remove leading / trailing spaces (Intel pads ProcessorNameString).
 * @param text Input.
 * @return Trimmed copy.
 */
std::string trimSpaces(const std::string& text)
{
    const auto first = text.find_first_not_of(' ');
    if (first == std::string::npos)
    {
        return {};
    }
    return text.substr(first, text.find_last_not_of(' ') - first + 1);
}

/**
 * @brief Fill sockets, cores, logical processors and cache sizes.
 * @param info Out.
 */
void readTopology(CpuStaticInfo& info)
{
    DWORD bytes = 0;
    GetLogicalProcessorInformationEx(RelationAll, nullptr, &bytes);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes == 0)
    {
        return;
    }
    std::vector<std::byte> buffer(bytes);
    auto* first = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data());
    if (!GetLogicalProcessorInformationEx(RelationAll, first, &bytes))
    {
        return;
    }

    std::uint32_t sockets = 0;
    std::uint32_t cores = 0;
    std::uint32_t logical = 0;
    std::map<BYTE, std::uint64_t> cacheBytes;
    // Variable-size records: each one starts Size bytes after the previous one.
    for (DWORD offset = 0; offset < bytes;)
    {
        const auto* entry = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data() + offset);
        switch (entry->Relationship)
        {
        case RelationProcessorPackage:
            ++sockets;
            break;
        case RelationProcessorCore:
            ++cores;
            for (WORD g = 0; g < entry->Processor.GroupCount; ++g)
            {
                logical += static_cast<std::uint32_t>(
                    std::popcount(static_cast<unsigned long long>(entry->Processor.GroupMask[g].Mask)));
            }
            break;
        case RelationCache:
            cacheBytes[entry->Cache.Level] += entry->Cache.CacheSize;
            break;
        default:
            break;
        }
        if (entry->Size == 0)
        {
            break;
        }
        offset += entry->Size;
    }

    if (sockets > 0)
    {
        info.sockets = sockets;
    }
    if (cores > 0)
    {
        info.cores = cores;
    }
    if (logical > 0)
    {
        info.logicalProcessors = logical;
    }
    const auto level = [&cacheBytes](BYTE n) -> std::optional<std::uint64_t> {
        const auto found = cacheBytes.find(n);
        return found == cacheBytes.end() ? std::nullopt : std::optional<std::uint64_t>(found->second);
    };
    info.l1CacheBytes = level(1);
    info.l2CacheBytes = level(2);
    info.l3CacheBytes = level(3);
}

/**
 * @brief Call NtQuerySystemInformation with a buffer that grows until the data fits.
 * @param infoClass Information class.
 * @param initialBytes First buffer size.
 * @return Buffer holding the data.
 * @throws std::runtime_error when the call keeps failing.
 */
std::vector<std::byte> querySystemInformation(SYSTEM_INFORMATION_CLASS infoClass, ULONG initialBytes)
{
    std::vector<std::byte> buffer(initialBytes);
    for (int attempt = 0; attempt < kMaxNtQueryAttempts; ++attempt)
    {
        ULONG needed = 0;
        const NTSTATUS status =
            NtQuerySystemInformation(infoClass, buffer.data(), static_cast<ULONG>(buffer.size()), &needed);
        if (status >= 0)
        {
            buffer.resize(needed > 0 && needed <= buffer.size() ? needed : buffer.size());
            return buffer;
        }
        if (status != kStatusInfoLengthMismatch)
        {
            break;
        }
        buffer.resize(std::max<std::size_t>(needed, buffer.size() * 2) + kProcessBufferSlackBytes);
    }
    throw std::runtime_error("NtQuerySystemInformation(" + std::to_string(static_cast<int>(infoClass)) + ") failed");
}

} // namespace

/** PDH query and counter handles. A null counter means "not available on this machine". */
struct WindowsCpuProvider::PdhCounters
{
    PDH_HQUERY query = nullptr;
    PDH_HCOUNTER contextSwitches = nullptr;
    PDH_HCOUNTER interrupts = nullptr;
    PDH_HCOUNTER systemCalls = nullptr;
    PDH_HCOUNTER dpcs = nullptr;
    PDH_HCOUNTER performance = nullptr; ///< % Processor Performance (current clock / base clock).
    PDH_HCOUNTER thermal = nullptr;     ///< Thermal zones, wildcard instance, Kelvin.

    /**
     * @brief Open the query and add the counters; missing counters stay null.
     */
    PdhCounters()
    {
        if (PdhOpenQueryW(nullptr, 0, &query) != ERROR_SUCCESS)
        {
            query = nullptr;
            return;
        }
        // English names: PdhAddCounterW would need localized names on non-English Windows.
        contextSwitches = add(L"\\System\\Context Switches/sec");
        interrupts = add(L"\\Processor(_Total)\\Interrupts/sec");
        systemCalls = add(L"\\System\\System Calls/sec");
        dpcs = add(L"\\Processor(_Total)\\DPCs Queued/sec");
        performance = add(L"\\Processor Information(_Total)\\% Processor Performance");
        thermal = add(L"\\Thermal Zone Information(*)\\Temperature");
    }

    /** @brief Close the query (also frees its counters). */
    ~PdhCounters()
    {
        if (query != nullptr)
        {
            PdhCloseQuery(query);
        }
    }

    PdhCounters(const PdhCounters&) = delete;
    PdhCounters& operator=(const PdhCounters&) = delete;

    /**
     * @brief Add one counter to the query.
     * @param path English counter path.
     * @return Counter handle, or nullptr when the counter does not exist.
     */
    PDH_HCOUNTER add(const wchar_t* path) const
    {
        PDH_HCOUNTER counter = nullptr;
        return PdhAddEnglishCounterW(query, path, 0, &counter) == ERROR_SUCCESS ? counter : nullptr;
    }

    /**
     * @brief Raw cumulative value of a rate counter (e.g. total context switches since boot).
     * @param counter Counter or nullptr.
     * @return FirstValue of the raw counter, or empty when unavailable.
     */
    static std::optional<std::uint64_t> raw(PDH_HCOUNTER counter)
    {
        if (counter == nullptr)
        {
            return std::nullopt;
        }
        PDH_RAW_COUNTER value{};
        if (PdhGetRawCounterValue(counter, nullptr, &value) != ERROR_SUCCESS ||
            (value.CStatus != PDH_CSTATUS_VALID_DATA && value.CStatus != PDH_CSTATUS_NEW_DATA) || value.FirstValue < 0)
        {
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(value.FirstValue);
    }

    /**
     * @brief Formatted value of a counter (valid from the second collection for averaged counters).
     * @param counter Counter or nullptr.
     * @return Value, or empty when unavailable.
     */
    static std::optional<double> formatted(PDH_HCOUNTER counter)
    {
        if (counter == nullptr)
        {
            return std::nullopt;
        }
        PDH_FMT_COUNTERVALUE value{};
        if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, nullptr, &value) != ERROR_SUCCESS ||
            (value.CStatus != PDH_CSTATUS_VALID_DATA && value.CStatus != PDH_CSTATUS_NEW_DATA))
        {
            return std::nullopt;
        }
        return value.doubleValue;
    }

    /**
     * @brief Highest value over the instances of a wildcard counter.
     * @param counter Counter or nullptr.
     * @return Maximum, or empty when no instance has data.
     */
    static std::optional<double> maxOfInstances(PDH_HCOUNTER counter)
    {
        if (counter == nullptr)
        {
            return std::nullopt;
        }
        DWORD bytes = 0;
        DWORD count = 0;
        if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bytes, &count, nullptr) != static_cast<PDH_STATUS>(PDH_MORE_DATA) ||
            bytes == 0)
        {
            return std::nullopt;
        }
        std::vector<std::byte> buffer(bytes);
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
        if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bytes, &count, items) != ERROR_SUCCESS)
        {
            return std::nullopt;
        }
        std::optional<double> highest;
        for (DWORD i = 0; i < count; ++i)
        {
            const auto& v = items[i].FmtValue;
            if (v.CStatus == PDH_CSTATUS_VALID_DATA || v.CStatus == PDH_CSTATUS_NEW_DATA)
            {
                highest = highest ? std::max(*highest, v.doubleValue) : v.doubleValue;
            }
        }
        return highest;
    }
};

/**
 * @brief Open the PDH query and read the base clock from the registry.
 *        PDH failures are not fatal: the affected values are reported as unavailable.
 */
WindowsCpuProvider::WindowsCpuProvider()
    : m_pdh(std::make_unique<PdhCounters>())
{
    if (const auto mhz = readRegistryDword(kProcessorRegistryKey, L"~MHz"); mhz && *mhz > 0)
    {
        m_baseSpeedMhz = static_cast<double>(*mhz);
    }
}

/** @brief Close the PDH query. Out of line because PdhCounters is incomplete in the header. */
WindowsCpuProvider::~WindowsCpuProvider() = default;

/**
 * @brief Read model, topology, base clock, caches and virtualization state.
 * @return Static info (max clock is not reported by Windows: empty).
 */
CpuStaticInfo WindowsCpuProvider::getStaticInfo() const
{
    CpuStaticInfo info;
    info.model = trimSpaces(toUtf8(readRegistryString(kProcessorRegistryKey, L"ProcessorNameString")));
    if (info.model.empty())
    {
        info.model = "Unknown processor";
    }

    SYSTEM_INFO system{};
    GetNativeSystemInfo(&system);
    info.logicalProcessors = std::max<std::uint32_t>(system.dwNumberOfProcessors, 1);
    info.cores = info.logicalProcessors;
    readTopology(info);

    info.baseSpeedMhz = m_baseSpeedMhz;
    info.virtualization =
        IsProcessorFeaturePresent(kPfVirtFirmwareEnabled) ? Virtualization::Enabled : Virtualization::Disabled;
    info.countersSource = toUtf8(kCountersSourceText);
    return info;
}

/**
 * @brief Read all cumulative counters and gauges.
 * @return Counters.
 * @throws std::runtime_error when NtQuerySystemInformation fails.
 */
CpuCounters WindowsCpuProvider::readCounters()
{
    CpuCounters counters;
    readProcessorTimes(counters);
    readProcesses(counters);
    readPdh(counters);

    const std::uint64_t uptimeMs = GetTickCount64();
    counters.uptimeSeconds = uptimeMs / kMillisecondsPerSecond;
    counters.bootTimeUnixSeconds = static_cast<std::int64_t>(std::time(nullptr)) -
                                   static_cast<std::int64_t>(counters.uptimeSeconds);
    counters.processTimeUnitsPerSecond = kHundredNanosecondsPerSecond;
    return counters;
}

/**
 * @brief Per-processor and total CPU times (processor group 0).
 * @param counters Out: total, perCore.
 * @throws std::runtime_error when the query fails.
 */
void WindowsCpuProvider::readProcessorTimes(CpuCounters& counters) const
{
    const auto buffer = querySystemInformation(kSystemProcessorPerformanceInformation,
                                               static_cast<ULONG>(kMaxGroupProcessors * sizeof(ProcessorPerformance)));
    const std::size_t count = buffer.size() / sizeof(ProcessorPerformance);
    const auto* entries = reinterpret_cast<const ProcessorPerformance*>(buffer.data());
    for (std::size_t i = 0; i < count; ++i)
    {
        const auto& p = entries[i];
        CpuTimes times;
        times.idle = toUnsigned(p.idleTime);
        times.user = toUnsigned(p.userTime);
        times.dpc = toUnsigned(p.dpcTime);
        times.interrupt = toUnsigned(p.interruptTime);
        // KernelTime includes idle, DPC and interrupt time; the provider contract wants disjoint parts.
        const std::uint64_t kernel = toUnsigned(p.kernelTime);
        const std::uint64_t included = times.idle + times.dpc + times.interrupt;
        times.kernel = kernel > included ? kernel - included : 0;

        counters.perCore.push_back(times);
        counters.total.user += times.user;
        counters.total.kernel += times.kernel;
        counters.total.dpc += times.dpc;
        counters.total.interrupt += times.interrupt;
        counters.total.idle += times.idle;
    }
}

/**
 * @brief Process, thread and handle counts and per-process counters.
 * @param counters Out: processCount, threadCount, handleCount, processes.
 * @throws std::runtime_error when the query fails.
 */
void WindowsCpuProvider::readProcesses(CpuCounters& counters) const
{
    const auto buffer = querySystemInformation(kSystemProcessInformation, kInitialProcessBufferBytes);
    std::uint64_t handles = 0;
    for (std::size_t offset = 0; offset + sizeof(ProcessEntry) <= buffer.size();)
    {
        const auto* entry = reinterpret_cast<const ProcessEntry*>(buffer.data() + offset);
        const auto pid = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(entry->uniqueProcessId));
        // pid 0 is the System Idle Process: its "CPU time" is idle time, not a process.
        if (pid != 0)
        {
            ++counters.processCount;
            counters.threadCount += entry->numberOfThreads;
            handles += entry->handleCount;
            const std::wstring_view name(entry->imageName.Buffer, entry->imageName.Length / sizeof(wchar_t));
            counters.processes.push_back(providers::cpu::ProcessCounters{
                pid,
                toUtf8(name),
                toUnsigned(entry->userTime) + toUnsigned(entry->kernelTime),
                toUnsigned(entry->workingSetPrivateSize),
            });
        }
        if (entry->nextEntryOffset == 0)
        {
            break;
        }
        offset += entry->nextEntryOffset;
    }
    counters.handleCount = handles;
}

/**
 * @brief Scheduler counters, current clock and temperature from PDH.
 * @param counters Out: contextSwitches, interrupts, systemCalls, dpcs, speedMhz, temperatureCelsius.
 */
void WindowsCpuProvider::readPdh(CpuCounters& counters)
{
    if (m_pdh->query == nullptr || PdhCollectQueryData(m_pdh->query) != ERROR_SUCCESS)
    {
        return;
    }
    counters.contextSwitches = PdhCounters::raw(m_pdh->contextSwitches);
    counters.interrupts = PdhCounters::raw(m_pdh->interrupts);
    counters.systemCalls = PdhCounters::raw(m_pdh->systemCalls);
    counters.dpcs = PdhCounters::raw(m_pdh->dpcs);

    // Task Manager's "Speed": base clock scaled by % Processor Performance (> 100 under turbo).
    if (const auto performance = PdhCounters::formatted(m_pdh->performance); performance && m_baseSpeedMhz)
    {
        counters.speedMhz = *m_baseSpeedMhz * *performance / kFullPercent;
    }
    if (const auto kelvin = PdhCounters::maxOfInstances(m_pdh->thermal); kelvin && *kelvin > 0)
    {
        counters.temperatureCelsius = *kelvin - kKelvinToCelsius;
    }
}

} // namespace systemlens::plugin
