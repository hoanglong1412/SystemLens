/**
 * @file    WindowsProcessProvider.cpp
 * @brief   Implementation of WindowsProcessProvider.
 */
#include "providers/windows/WindowsProcessProvider.h"

#include "providers/ProviderError.h"
#include "providers/windows/NativeApi.h"
#include "providers/windows/WindowsUtil.h"

#include <string_view>

namespace systemlens::providers::windows_os {

namespace {

/** PID of the "System Idle Process" pseudo-process, whose CPU time is the idle time. */
constexpr std::uint32_t kIdleProcessId = 0;
/** First buffer size; grows when the system reports that more is needed. */
constexpr std::size_t kInitialBufferBytes = 512 * 1024;
/** Extra room added on top of the size the system asks for: processes may start between two calls. */
constexpr std::size_t kBufferSlackBytes = 64 * 1024;
constexpr int kMaxQueryAttempts = 5;

/**
 * @brief Convert a process or parent id stored as a HANDLE to a number.
 * @param id UniqueProcessId / InheritedFromUniqueProcessId.
 * @return The PID (PIDs fit in 32 bits).
 */
std::uint32_t toPid(HANDLE id)
{
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(id));
}

} // namespace

/**
 * @brief Snapshot all processes with one system call.
 *
 * NtQuerySystemInformation returns CPU times and memory of every process, including
 * protected system processes that OpenProcess refuses to open.
 * @return One sample per process, the idle process excluded.
 * @throws ProviderError if the query keeps failing.
 */
std::vector<ProcessSample> WindowsProcessProvider::readProcesses()
{
    if (m_buffer.empty())
    {
        m_buffer.resize(kInitialBufferBytes / sizeof(std::max_align_t));
    }

    NTSTATUS status = native::kStatusInfoLengthMismatch;
    for (int attempt = 0; attempt < kMaxQueryAttempts && status == native::kStatusInfoLengthMismatch; ++attempt)
    {
        ULONG neededBytes = 0;
        status = ::NtQuerySystemInformation(SystemProcessInformation, m_buffer.data(),
                                            static_cast<ULONG>(m_buffer.size() * sizeof(std::max_align_t)),
                                            &neededBytes);
        if (status == native::kStatusInfoLengthMismatch)
        {
            m_buffer.resize((neededBytes + kBufferSlackBytes) / sizeof(std::max_align_t) + 1);
        }
    }
    if (!NT_SUCCESS(status))
    {
        throw ProviderError("NtQuerySystemInformation(SystemProcessInformation) failed");
    }

    std::vector<ProcessSample> processes;
    const auto* cursor = reinterpret_cast<const std::byte*>(m_buffer.data());
    for (;;)
    {
        const auto* entry = reinterpret_cast<const native::ProcessInformation*>(cursor);
        const std::uint32_t pid = toPid(entry->UniqueProcessId);
        if (pid != kIdleProcessId)
        {
            const std::wstring_view imageName = entry->ImageName.Buffer != nullptr
                ? std::wstring_view(entry->ImageName.Buffer, entry->ImageName.Length / sizeof(wchar_t))
                : std::wstring_view();
            processes.push_back(ProcessSample{
                .pid = pid,
                .parentPid = toPid(entry->InheritedFromUniqueProcessId),
                .name = toUtf8(imageName),
                .cpuTime = fromHundredNanoseconds(entry->UserTime.QuadPart + entry->KernelTime.QuadPart),
                .memoryBytes = static_cast<std::uint64_t>(entry->WorkingSetPrivateSize.QuadPart),
            });
        }
        if (entry->NextEntryOffset == 0)
        {
            break;
        }
        cursor += entry->NextEntryOffset;
    }
    return processes;
}

/**
 * @brief PID of the backend.
 * @return GetCurrentProcessId().
 */
std::uint32_t WindowsProcessProvider::currentProcessId() const
{
    return ::GetCurrentProcessId();
}

} // namespace systemlens::providers::windows_os
