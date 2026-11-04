/**
 * @file    WindowsSystemInfoProvider.cpp
 * @brief   Implementation of WindowsSystemInfoProvider.
 */
#include "WindowsSystemInfoProvider.h"

#include "WideString.h"

#include <windows.h>
#include <lmcons.h>
#include <winternl.h>

#include <iterator>
#include <string>
#include <string_view>

// RtlGetVersion is exported by ntdll but not declared by every SDK header. Unlike GetVersionEx it
// reports the real version even when the executable has no compatibility manifest.
extern "C" NTSTATUS NTAPI RtlGetVersion(PRTL_OSVERSIONINFOW versionInformation);

namespace systemlens::plugin {

namespace {

constexpr const wchar_t* kCurrentVersionKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
/** First build of Windows 11; its ProductName still says "Windows 10". */
constexpr DWORD kFirstWindows11Build = 22000;
constexpr std::string_view kWindows10 = "Windows 10";
constexpr std::string_view kWindows11 = "Windows 11";

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
 * @brief Short name of the native processor architecture.
 * @return "x64", "ARM64", "x86" or empty.
 */
std::string architecture()
{
    SYSTEM_INFO info{};
    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return "x64";
    case PROCESSOR_ARCHITECTURE_ARM64:
        return "ARM64";
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "x86";
    default:
        return {};
    }
}

} // namespace

/**
 * @brief Read computer name, edition, version and user.
 * @return Identity; unknown parts are empty strings.
 */
providers::system::SystemIdentity WindowsSystemInfoProvider::getIdentity() const
{
    providers::system::SystemIdentity identity;
    identity.platform = "windows";

    wchar_t computer[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD computerLength = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(computer, &computerLength))
    {
        identity.hostName = toUtf8(std::wstring_view(computer, computerLength));
    }

    wchar_t user[UNLEN + 1] = {};
    DWORD userLength = static_cast<DWORD>(std::size(user));
    if (GetUserNameW(user, &userLength) && userLength > 0)
    {
        identity.userName = toUtf8(std::wstring_view(user, userLength - 1));
    }

    RTL_OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    const bool haveVersion = RtlGetVersion(&version) >= 0;

    std::string product = toUtf8(readRegistryString(kCurrentVersionKey, L"ProductName"));
    if (product.empty())
    {
        product = "Windows";
    }
    if (haveVersion && version.dwBuildNumber >= kFirstWindows11Build && product.rfind(kWindows10, 0) == 0)
    {
        product.replace(0, kWindows10.size(), kWindows11);
    }
    const std::string arch = architecture();
    identity.osName = arch.empty() ? product : product + " " + arch;

    if (haveVersion)
    {
        identity.osVersion = "Version " + std::to_string(version.dwMajorVersion) + "." +
                             std::to_string(version.dwMinorVersion) + "." + std::to_string(version.dwBuildNumber);
    }
    return identity;
}

} // namespace systemlens::plugin
