/**
 * @file    WindowsCpuProvider.cpp
 * @brief   Implementation of WindowsCpuProvider.
 */
#include "providers/windows/WindowsCpuProvider.h"

#include "providers/ProviderError.h"
#include "providers/windows/NativeApi.h"
#include "providers/windows/WindowsUtil.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <vector>

namespace systemlens::providers::windows_os {

namespace {

constexpr const wchar_t* kProcessorKey = L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
constexpr const char* kActivitySource = "PDH \\System, \\Processor(_Total)";
constexpr double kMaxPercent = 100.0;
/** The Thermal Zone Information counter reports kelvin. */
constexpr double kKelvinOffset = 273.15;

/**
 * @brief Remove leading and trailing spaces (the registry model string is padded).
 * @param text Text to trim.
 * @return Trimmed copy.
 */
std::wstring trimSpaces(const std::wstring& text)
{
    const auto first = text.find_first_not_of(L' ');
    if (first == std::wstring::npos)
    {
        return {};
    }
    return text.substr(first, text.find_last_not_of(L' ') - first + 1);
}

/**
 * @brief Read sockets, cores, logical processors and cache sizes.
 * @param info Receives the topology and cache fields.
 * @throws ProviderError if GetLogicalProcessorInformationEx fails.
 */
void readTopology(CpuStaticInfo& info)
{
    DWORD bytes = 0;
    ::GetLogicalProcessorInformationEx(RelationAll, nullptr, &bytes);
    if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes == 0)
    {
        throw ProviderError("GetLogicalProcessorInformationEx failed");
    }
    std::vector<std::max_align_t> buffer(bytes / sizeof(std::max_align_t) + 1);
    auto* first = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data());
    if (!::GetLogicalProcessorInformationEx(RelationAll, first, &bytes))
    {
        throw ProviderError("GetLogicalProcessorInformationEx failed");
    }

    // The buffer is a sequence of variable-size records; each one stores its own size.
    const auto* cursor = reinterpret_cast<const std::byte*>(first);
    const auto* const end = cursor + bytes;
    while (cursor < end)
    {
        const auto* record = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(cursor);
        switch (record->Relationship)
        {
        case RelationProcessorPackage:
            ++info.sockets;
            break;
        case RelationProcessorCore:
            ++info.cores;
            for (WORD group = 0; group < record->Processor.GroupCount; ++group)
            {
                info.logicalProcessors +=
                    static_cast<std::uint32_t>(std::popcount(record->Processor.GroupMask[group].Mask));
            }
            break;
        case RelationCache:
        {
            std::optional<std::uint64_t>* total = record->Cache.Level == 1 ? &info.l1CacheBytes
                                                  : record->Cache.Level == 2 ? &info.l2CacheBytes
                                                  : record->Cache.Level == 3 ? &info.l3CacheBytes
                                                                             : nullptr;
            if (total != nullptr)
            {
                *total = total->value_or(0) + record->Cache.CacheSize;
            }
            break;
        }
        default:
            break;
        }
        cursor += record->Size;
    }
}

/**
 * @brief Convert the counters of one processor (or the sum of several) to CPU times.
 * @param idle      IdleTime, 100 ns units.
 * @param kernel    KernelTime (includes idle, DPC and interrupt time).
 * @param user      UserTime.
 * @param dpc       DpcTime.
 * @param interrupt InterruptTime.
 * @return Disjoint parts: Windows counts idle, DPC and interrupt time as kernel time, so they are subtracted.
 */
CpuTimes toCpuTimes(std::int64_t idle, std::int64_t kernel, std::int64_t user, std::int64_t dpc,
                    std::int64_t interrupt)
{
    return CpuTimes{
        .user = fromHundredNanoseconds(user),
        .kernel = fromHundredNanoseconds(std::max<std::int64_t>(kernel - idle - dpc - interrupt, 0)),
        .dpc = fromHundredNanoseconds(dpc),
        .interrupt = fromHundredNanoseconds(interrupt),
        .idle = fromHundredNanoseconds(idle),
    };
}

/** CPU times of all processors together and of each one. */
struct ProcessorTimes
{
    CpuTimes total;
    std::vector<CpuTimes> perCore;
};

/**
 * @brief Read the cumulative CPU times.
 * @return Times of every logical processor and their sum.
 * @throws ProviderError if the native query fails.
 */
ProcessorTimes readTimes()
{
    SYSTEM_INFO system{};
    ::GetSystemInfo(&system);
    // Covers the processor group of this process (up to 64 logical processors).
    std::vector<native::ProcessorPerformanceInformation> processors(system.dwNumberOfProcessors);
    ULONG returnedBytes = 0;
    const NTSTATUS status = ::NtQuerySystemInformation(
        SystemProcessorPerformanceInformation, processors.data(),
        static_cast<ULONG>(processors.size() * sizeof(native::ProcessorPerformanceInformation)), &returnedBytes);
    if (!NT_SUCCESS(status))
    {
        throw ProviderError("NtQuerySystemInformation(SystemProcessorPerformanceInformation) failed");
    }
    processors.resize(returnedBytes / sizeof(native::ProcessorPerformanceInformation));

    std::int64_t idle = 0;
    std::int64_t kernel = 0;
    std::int64_t user = 0;
    std::int64_t dpc = 0;
    std::int64_t interrupt = 0;
    ProcessorTimes times;
    times.perCore.reserve(processors.size());
    for (const native::ProcessorPerformanceInformation& processor : processors)
    {
        idle += processor.IdleTime.QuadPart;
        kernel += processor.KernelTime.QuadPart;
        user += processor.UserTime.QuadPart;
        dpc += processor.DpcTime.QuadPart;
        interrupt += processor.InterruptTime.QuadPart;
        times.perCore.push_back(toCpuTimes(processor.IdleTime.QuadPart, processor.KernelTime.QuadPart,
                                           processor.UserTime.QuadPart, processor.DpcTime.QuadPart,
                                           processor.InterruptTime.QuadPart));
    }
    times.total = toCpuTimes(idle, kernel, user, dpc, interrupt);
    return times;
}

} // namespace

/** @brief Register the PDH counters; counters missing on this machine simply stay empty. */
WindowsCpuProvider::WindowsCpuProvider()
    : m_contextSwitches(m_query.addCounter(L"\\System\\Context Switches/sec"))
    , m_systemCalls(m_query.addCounter(L"\\System\\System Calls/sec"))
    , m_interrupts(m_query.addCounter(L"\\Processor(_Total)\\Interrupts/sec"))
    , m_dpcs(m_query.addCounter(L"\\Processor(_Total)\\DPCs Queued/sec"))
    , m_performancePercent(m_query.addCounter(L"\\Processor Information(_Total)\\% Processor Performance"))
    , m_thermalZones(m_query.addCounter(L"\\Thermal Zone Information(*)\\Temperature"))
{
    if (const auto mhz = readRegistryDword(HKEY_LOCAL_MACHINE, kProcessorKey, L"~MHz"))
    {
        m_baseSpeedMhz = static_cast<double>(*mhz);
    }
}

/**
 * @brief Read model, topology, base clock, virtualization state and cache sizes.
 * @return Static information. maxSpeedMhz stays empty: Windows does not report the turbo ceiling.
 * @throws ProviderError if the topology cannot be read.
 */
CpuStaticInfo WindowsCpuProvider::readStaticInfo() const
{
    CpuStaticInfo info;
    info.model = toUtf8(trimSpaces(
        readRegistryString(HKEY_LOCAL_MACHINE, kProcessorKey, L"ProcessorNameString").value_or(L"Unknown processor")));
    readTopology(info);
    info.baseSpeedMhz = m_baseSpeedMhz;
    // Reports false when a hypervisor (e.g. Hyper-V, WSL 2) already owns the virtualization
    // extensions, even though they are enabled in the firmware.
    info.virtualization = ::IsProcessorFeaturePresent(PF_VIRT_FIRMWARE_ENABLED) ? VirtualizationState::Enabled
                                                                                : VirtualizationState::Disabled;
    info.activitySource = kActivitySource;
    return info;
}

/**
 * @brief Read CPU times, event counters, clock and temperature.
 *
 * The clock is "% Processor Performance" (average over the interval since the previous call)
 * applied to the base speed, which is how Task Manager computes "Speed".
 * @return Current sample; frequencyMhz is empty on the first call and where the counter is missing.
 * @throws ProviderError if the CPU times cannot be read.
 */
CpuSample WindowsCpuProvider::readSample()
{
    CpuSample sample;
    ProcessorTimes times = readTimes();
    sample.times = times.total;
    sample.perCore = std::move(times.perCore);

    if (!m_query.collect())
    {
        return sample;
    }
    // For "/sec" counters the raw value is the cumulative count; MetricsCollector derives the rate.
    sample.activity = CpuActivityCounters{
        .contextSwitches = m_query.rawValue(m_contextSwitches),
        .interrupts = m_query.rawValue(m_interrupts),
        .systemCalls = m_query.rawValue(m_systemCalls),
        .dpcs = m_query.rawValue(m_dpcs),
    };

    const auto performancePercent = m_query.formattedValue(m_performancePercent);
    if (performancePercent && m_baseSpeedMhz)
    {
        sample.frequencyMhz = *m_baseSpeedMhz * *performancePercent / kMaxPercent;
    }

    // Several ACPI thermal zones may exist; the hottest one is the closest to the CPU package.
    for (const auto& [zone, kelvin] : m_query.formattedInstances(m_thermalZones))
    {
        const double celsius = kelvin - kKelvinOffset;
        if (celsius > 0.0)
        {
            sample.temperatureCelsius = std::max(sample.temperatureCelsius.value_or(celsius), celsius);
        }
    }
    return sample;
}

} // namespace systemlens::providers::windows_os
