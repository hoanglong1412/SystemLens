/**
 * @file    WindowsProcessProvider.cpp
 * @brief   Implementation of WindowsProcessProvider.
 */
#include "providers/windows/WindowsProcessProvider.h"

#include "providers/ProviderError.h"
#include "providers/windows/NativeApi.h"
#include "providers/windows/WindowsUtil.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <unordered_set>

namespace systemlens::providers::windows_os {

namespace {

/** PID of the "System Idle Process" pseudo-process, whose CPU time is the idle time. */
constexpr std::uint32_t kIdleProcessId = 0;
/** First buffer size; grows when the system reports that more is needed. */
constexpr std::size_t kInitialBufferBytes = 512 * 1024;
/** Extra room added on top of the size the system asks for: processes may start between two calls. */
constexpr std::size_t kBufferSlackBytes = 64 * 1024;
constexpr int kMaxQueryAttempts = 5;

/** Largest account name + domain the owner lookup supports. */
constexpr DWORD kMaxAccountNameChars = 256;
/** Buffer for a TOKEN_USER: the structure plus the SID it points to (a SID is at most 68 bytes). */
constexpr std::size_t kTokenUserBufferBytes = 256;

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
 * @brief Find the account that owns a process.
 *
 * Opens the process with the least access that allows reading its token. Protected and system
 * processes refuse that, so their owner stays empty. Results are cached per PID.
 * @param pid Process id.
 * @return "DOMAIN\\user", or empty when the owner cannot be read.
 */
std::string WindowsProcessProvider::lookUpOwner(std::uint32_t pid)
{
    if (const auto cached = m_owners.find(pid); cached != m_owners.end())
    {
        return cached->second;
    }

    std::string owner;
    const UniqueHandle process(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
    HANDLE rawToken = nullptr;
    if (process && ::OpenProcessToken(process.get(), TOKEN_QUERY, &rawToken))
    {
        const UniqueHandle token(rawToken);
        std::array<std::byte, kTokenUserBufferBytes> buffer{};
        DWORD returned = 0;
        if (::GetTokenInformation(token.get(), TokenUser, buffer.data(), static_cast<DWORD>(buffer.size()),
                                  &returned))
        {
            std::array<wchar_t, kMaxAccountNameChars> name{};
            std::array<wchar_t, kMaxAccountNameChars> domain{};
            DWORD nameChars = kMaxAccountNameChars;
            DWORD domainChars = kMaxAccountNameChars;
            SID_NAME_USE use{};
            const auto* user = reinterpret_cast<const TOKEN_USER*>(buffer.data());
            if (::LookupAccountSidW(nullptr, user->User.Sid, name.data(), &nameChars, domain.data(), &domainChars,
                                    &use))
            {
                owner = toUtf8(std::wstring_view(domain.data(), domainChars)) + "\\"
                        + toUtf8(std::wstring_view(name.data(), nameChars));
            }
        }
    }
    m_owners.emplace(pid, owner);
    return owner;
}

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
                .userName = lookUpOwner(pid),
                .threadCount = entry->NumberOfThreads,
                // Windows has no per-process run state.
                .status = {},
                .commandLine = {},
            });
        }
        if (entry->NextEntryOffset == 0)
        {
            break;
        }
        cursor += entry->NextEntryOffset;
    }
    // Forget the owners of processes that exited, so a reused PID is looked up again.
    std::unordered_set<std::uint32_t> alive;
    alive.reserve(processes.size());
    for (const ProcessSample& process : processes)
    {
        alive.insert(process.pid);
    }
    std::erase_if(m_owners, [&alive](const auto& entry) { return !alive.contains(entry.first); });
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
