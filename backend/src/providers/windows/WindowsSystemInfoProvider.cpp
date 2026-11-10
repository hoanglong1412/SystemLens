/**
 * @file    WindowsSystemInfoProvider.cpp
 * @brief   Implementation of WindowsSystemInfoProvider.
 */
#include "providers/windows/WindowsSystemInfoProvider.h"

#include "providers/ProviderError.h"
#include "providers/windows/WindowsUtil.h"

#include <windows.h>

#include <lmcons.h>
#include <psapi.h>
#include <winsvc.h>
#include <wtsapi32.h>

#include <array>
#include <cstddef>
#include <cwchar>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

namespace systemlens::providers::windows_os {

namespace {

constexpr const wchar_t* kCurrentVersionKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
/**
 * Windows 11 still reports "Windows 10 ..." in ProductName (kept for compatibility);
 * the build number is the documented way to tell them apart.
 */
constexpr unsigned long kFirstWindows11Build = 22000;
constexpr std::wstring_view kWindows10Prefix = L"Windows 10";
constexpr std::wstring_view kWindows11Prefix = L"Windows 11";

/** Closes a Service Control Manager handle. */
struct ServiceHandleCloser
{
    void operator()(SC_HANDLE handle) const noexcept { ::CloseServiceHandle(handle); }
};
using UniqueServiceHandle = std::unique_ptr<std::remove_pointer_t<SC_HANDLE>, ServiceHandleCloser>;

/**
 * @brief Read the NetBIOS computer name.
 * @return e.g. "DESKTOP-ABC123"; empty on failure.
 */
std::string readHostName()
{
    std::array<wchar_t, MAX_COMPUTERNAME_LENGTH + 1> buffer{};
    DWORD size = static_cast<DWORD>(buffer.size());
    return ::GetComputerNameW(buffer.data(), &size) ? toUtf8(std::wstring_view(buffer.data(), size)) : std::string();
}

/**
 * @brief Read the name of the user the backend runs as.
 * @return User name; empty on failure.
 */
std::string readUserName()
{
    std::array<wchar_t, UNLEN + 1> buffer{};
    DWORD size = static_cast<DWORD>(buffer.size());
    // On success size counts the terminating NUL.
    return (::GetUserNameW(buffer.data(), &size) && size > 0) ? toUtf8(std::wstring_view(buffer.data(), size - 1))
                                                               : std::string();
}

/**
 * @brief Read the window station name of the current session.
 * @return "Console" for the physical console, "RDP-Tcp#N" for remote desktop; empty on failure.
 */
std::string readSessionName()
{
    wchar_t* name = nullptr;
    DWORD bytes = 0;
    if (!::WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSWinStationName, &name,
                                       &bytes)
        || name == nullptr)
    {
        return {};
    }
    std::string result = toUtf8(name);
    ::WTSFreeMemory(name);
    return result;
}

/**
 * @brief Name the native processor architecture (not the one of this process).
 * @return "x64", "arm64", "x86", or "unknown".
 */
std::string readArchitecture()
{
    SYSTEM_INFO info{};
    ::GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return "x64";
    case PROCESSOR_ARCHITECTURE_ARM64:
        return "arm64";
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "x86";
    default:
        return "unknown";
    }
}

/**
 * @brief Count the installed Win32 services (running or not).
 * @return Number of services, or std::nullopt when the Service Control Manager refuses the query.
 */
std::optional<std::uint32_t> countServices()
{
    const UniqueServiceHandle manager(::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE));
    if (!manager)
    {
        return std::nullopt;
    }

    std::uint32_t total = 0;
    DWORD resumeHandle = 0;
    std::vector<std::max_align_t> buffer;
    for (;;)
    {
        DWORD bytesNeeded = 0;
        DWORD returned = 0;
        const DWORD bufferBytes = static_cast<DWORD>(buffer.size() * sizeof(std::max_align_t));
        const BOOL done = ::EnumServicesStatusExW(manager.get(), SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                                                  SERVICE_STATE_ALL, reinterpret_cast<LPBYTE>(buffer.data()),
                                                  bufferBytes, &bytesNeeded, &returned, &resumeHandle, nullptr);
        total += returned;
        if (done)
        {
            return total;
        }
        if (::GetLastError() != ERROR_MORE_DATA)
        {
            return std::nullopt;
        }
        // The call returned what fitted and advanced resumeHandle; grow the buffer for the rest.
        buffer.resize((bufferBytes + bytesNeeded) / sizeof(std::max_align_t) + 1);
    }
}

} // namespace

/**
 * @brief Read computer name, Windows edition and version, architecture, user and session.
 * @return Identity; individual fields are empty when their API call fails.
 */
SystemIdentity WindowsSystemInfoProvider::readIdentity() const
{
    const std::wstring buildText =
        readRegistryString(HKEY_LOCAL_MACHINE, kCurrentVersionKey, L"CurrentBuildNumber").value_or(L"0");
    const unsigned long build = std::wcstoul(buildText.c_str(), nullptr, 10);
    const auto major = readRegistryDword(HKEY_LOCAL_MACHINE, kCurrentVersionKey, L"CurrentMajorVersionNumber");
    const auto minor = readRegistryDword(HKEY_LOCAL_MACHINE, kCurrentVersionKey, L"CurrentMinorVersionNumber");

    std::wstring product =
        readRegistryString(HKEY_LOCAL_MACHINE, kCurrentVersionKey, L"ProductName").value_or(L"Windows");
    if (build >= kFirstWindows11Build && product.starts_with(kWindows10Prefix))
    {
        product.replace(0, kWindows10Prefix.size(), kWindows11Prefix);
    }

    return SystemIdentity{
        .hostName = readHostName(),
        .osName = toUtf8(product),
        .osVersion = std::to_string(major.value_or(0)) + '.' + std::to_string(minor.value_or(0)) + '.'
                     + std::to_string(build),
        .architecture = readArchitecture(),
        .userName = readUserName(),
        .sessionName = readSessionName(),
    };
}

/**
 * @brief Compute the boot time as "now minus the time since boot".
 * @return Boot time rounded to seconds (hibernation time counts as uptime, as in Task Manager).
 */
std::chrono::system_clock::time_point WindowsSystemInfoProvider::readBootTime() const
{
    const auto sinceBoot = std::chrono::milliseconds(::GetTickCount64());
    return std::chrono::round<std::chrono::seconds>(std::chrono::system_clock::now() - sinceBoot);
}

/**
 * @brief Read the process, thread, handle and service counts.
 * @return Counts.
 * @throws ProviderError if GetPerformanceInfo fails.
 */
SystemCounts WindowsSystemInfoProvider::readCounts() const
{
    PERFORMANCE_INFORMATION info{};
    info.cb = sizeof(info);
    if (!::GetPerformanceInfo(&info, sizeof(info)))
    {
        throw ProviderError("GetPerformanceInfo failed");
    }
    return SystemCounts{
        .processes = info.ProcessCount,
        .threads = info.ThreadCount,
        .handles = info.HandleCount,
        .services = countServices(),
    };
}

} // namespace systemlens::providers::windows_os
