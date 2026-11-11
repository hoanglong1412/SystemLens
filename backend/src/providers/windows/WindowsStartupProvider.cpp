/**
 * @file    WindowsStartupProvider.cpp
 * @brief   Implementation of WindowsStartupProvider.
 */
#include "providers/windows/WindowsStartupProvider.h"

#include "providers/windows/WindowsUtil.h"

#include <windows.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace systemlens::providers::windows_os {

namespace {

constexpr const wchar_t* kRunKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr const wchar_t* kRunKeyWow64 = L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr const wchar_t* kApprovedRunKey =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run";
constexpr const wchar_t* kApprovedFolderKey =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\StartupFolder";
constexpr const wchar_t* kStartupFolderSuffix = L"\\Microsoft\\Windows\\Start Menu\\Programs\\Startup";
constexpr const wchar_t* kUserFolderEnvVar = L"APPDATA";
constexpr const wchar_t* kAllUsersFolderEnvVar = L"ProgramData";
constexpr const wchar_t* kDesktopIniName = L"desktop.ini";
constexpr DWORD kMaxValueNameChars = 16383; ///< Documented maximum length of a registry value name.
constexpr DWORD kMaxValueDataBytes = 32768;
constexpr std::size_t kApprovedBlobMinBytes = 1;
/** In a StartupApproved blob, an even first byte means enabled and an odd one means disabled. */
constexpr unsigned char kDisabledBit = 0x01;

/**
 * @brief Tell whether Task Manager's "Startup" switch of an entry is on.
 * @param root      HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE.
 * @param approvedKey StartupApproved key (Run or StartupFolder).
 * @param name      Value name = entry name / file name.
 * @return False only when the entry was disabled; enabled when there is no record.
 */
bool isApproved(HKEY root, const wchar_t* approvedKey, const std::wstring& name)
{
    std::array<unsigned char, 16> blob{};
    DWORD bytes = static_cast<DWORD>(blob.size());
    if (::RegGetValueW(root, approvedKey, name.c_str(), RRF_RT_REG_BINARY, nullptr, blob.data(), &bytes)
            != ERROR_SUCCESS
        || bytes < kApprovedBlobMinBytes)
    {
        return true;
    }
    return (blob[0] & kDisabledBit) == 0;
}

/**
 * @brief Add the values of one Run key.
 * @param root     HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE.
 * @param runKey   Registry path of the Run key.
 * @param rootName "HKCU" or "HKLM" for the location text.
 * @param scope    Whose logon starts the entries.
 * @param items    Receives the entries.
 */
void collectRunKey(HKEY root, const wchar_t* runKey, const wchar_t* rootName, StartupScope scope,
                   std::vector<StartupItem>& items)
{
    HKEY rawKey = nullptr;
    if (::RegOpenKeyExW(root, runKey, 0, KEY_READ, &rawKey) != ERROR_SUCCESS)
    {
        return;
    }
    // RAII for the key: closed on every path out of the function.
    const std::unique_ptr<std::remove_pointer_t<HKEY>, decltype(&::RegCloseKey)> key(rawKey, &::RegCloseKey);

    std::wstring name(kMaxValueNameChars + 1, L'\0');
    std::wstring data(kMaxValueDataBytes / sizeof(wchar_t), L'\0');
    for (DWORD index = 0;; ++index)
    {
        DWORD nameChars = kMaxValueNameChars + 1;
        DWORD dataBytes = kMaxValueDataBytes;
        DWORD type = 0;
        const LSTATUS status = ::RegEnumValueW(key.get(), index, name.data(), &nameChars, nullptr, &type,
                                               reinterpret_cast<LPBYTE>(data.data()), &dataBytes);
        if (status == ERROR_NO_MORE_ITEMS)
        {
            return;
        }
        if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
        {
            continue;
        }
        const std::wstring valueName(name.data(), nameChars);
        // dataBytes includes the terminating NUL of the stored string.
        const std::size_t chars = dataBytes / sizeof(wchar_t);
        items.push_back(StartupItem{
            .name = toUtf8(valueName),
            .command = toUtf8(std::wstring_view(data.data(), chars > 0 ? chars - 1 : 0)),
            .location = toUtf8(std::wstring(rootName) + L"\\" + runKey),
            .scope = scope,
            .enabled = isApproved(root, kApprovedRunKey, valueName),
        });
    }
}

/**
 * @brief Add the files of one Startup folder.
 * @param envVar   Environment variable holding the base folder (APPDATA or ProgramData).
 * @param root     Registry root of the matching StartupApproved key.
 * @param scope    Whose logon starts the entries.
 * @param items    Receives the entries.
 */
void collectStartupFolder(const wchar_t* envVar, HKEY root, StartupScope scope, std::vector<StartupItem>& items)
{
    std::wstring base(MAX_PATH, L'\0');
    const DWORD length = ::GetEnvironmentVariableW(envVar, base.data(), static_cast<DWORD>(base.size()));
    if (length == 0 || length >= base.size())
    {
        return;
    }
    base.resize(length);
    const std::filesystem::path folder = base + kStartupFolderSuffix;

    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(folder, error))
    {
        const std::wstring fileName = entry.path().filename().wstring();
        if (!entry.is_regular_file(error) || fileName == kDesktopIniName)
        {
            continue;
        }
        items.push_back(StartupItem{
            .name = toUtf8(entry.path().stem().wstring()),
            .command = toUtf8(entry.path().wstring()),
            .location = toUtf8(folder.wstring()),
            .scope = scope,
            .enabled = isApproved(root, kApprovedFolderKey, fileName),
        });
    }
}

} // namespace

/**
 * @brief List what Windows starts at logon.
 * @return Entries of the Run keys (current user, all users, 32-bit view) and of both Startup folders.
 */
std::vector<StartupItem> WindowsStartupProvider::readStartupItems() const
{
    std::vector<StartupItem> items;
    collectRunKey(HKEY_CURRENT_USER, kRunKey, L"HKCU", StartupScope::User, items);
    collectRunKey(HKEY_LOCAL_MACHINE, kRunKey, L"HKLM", StartupScope::AllUsers, items);
    collectRunKey(HKEY_LOCAL_MACHINE, kRunKeyWow64, L"HKLM", StartupScope::AllUsers, items);
    collectStartupFolder(kUserFolderEnvVar, HKEY_CURRENT_USER, StartupScope::User, items);
    collectStartupFolder(kAllUsersFolderEnvVar, HKEY_LOCAL_MACHINE, StartupScope::AllUsers, items);
    return items;
}

} // namespace systemlens::providers::windows_os
