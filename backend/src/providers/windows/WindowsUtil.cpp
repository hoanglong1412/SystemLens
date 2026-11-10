/**
 * @file    WindowsUtil.cpp
 * @brief   Implementation of the helpers shared by the Windows providers.
 */
#include "providers/windows/WindowsUtil.h"

namespace systemlens::providers::windows_os {

namespace {

/** FILETIME-style durations count 100-nanosecond units. */
constexpr double kHundredNanosecondsPerSecond = 10'000'000.0;

} // namespace

/**
 * @brief Close a kernel handle.
 * @param handle Handle owned by a UniqueHandle (never null or INVALID_HANDLE_VALUE).
 */
void HandleCloser::operator()(HANDLE handle) const noexcept
{
    ::CloseHandle(handle);
}

/**
 * @brief Convert UTF-16 text from the Windows API to UTF-8.
 * @param text UTF-16 text.
 * @return UTF-8 text; empty when text is empty or cannot be converted.
 */
std::string toUtf8(std::wstring_view text)
{
    if (text.empty())
    {
        return {};
    }
    const int length = static_cast<int>(text.size());
    const int size = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), length, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
    {
        return {};
    }
    std::string utf8(static_cast<std::size_t>(size), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text.data(), length, utf8.data(), size, nullptr, nullptr);
    return utf8;
}

/**
 * @brief Convert a count of 100 ns units (FILETIME, LARGE_INTEGER times) to a duration.
 * @param ticks Number of 100 ns units.
 * @return Duration in seconds.
 */
std::chrono::duration<double> fromHundredNanoseconds(std::int64_t ticks)
{
    return std::chrono::duration<double>(static_cast<double>(ticks) / kHundredNanosecondsPerSecond);
}

/**
 * @brief Open a device or volume for metadata queries only.
 *
 * Desired access 0 is enough for the IOCTLs the providers use and, unlike read access,
 * does not require administrator rights.
 * @param path Device path, e.g. L"\\\\.\\PhysicalDrive0".
 * @return Owning handle; empty when the device does not exist or cannot be opened.
 */
UniqueHandle openForQuery(const std::wstring& path)
{
    const HANDLE handle = ::CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                        0, nullptr);
    return UniqueHandle(handle == INVALID_HANDLE_VALUE ? nullptr : handle);
}

/**
 * @brief Read a REG_SZ value.
 * @param root      Predefined key, e.g. HKEY_LOCAL_MACHINE.
 * @param subKey    Key path below root.
 * @param valueName Value name.
 * @return The text without the terminating NUL, or std::nullopt when missing or not a string.
 */
std::optional<std::wstring> readRegistryString(HKEY root, const wchar_t* subKey, const wchar_t* valueName)
{
    DWORD bytes = 0;
    if (::RegGetValueW(root, subKey, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &bytes) != ERROR_SUCCESS
        || bytes < sizeof(wchar_t))
    {
        return std::nullopt;
    }
    std::wstring text(bytes / sizeof(wchar_t), L'\0');
    if (::RegGetValueW(root, subKey, valueName, RRF_RT_REG_SZ, nullptr, text.data(), &bytes) != ERROR_SUCCESS)
    {
        return std::nullopt;
    }
    // bytes includes the terminating NUL that RegGetValueW guarantees.
    text.resize(bytes / sizeof(wchar_t) - 1);
    return text;
}

/**
 * @brief Read a REG_DWORD value.
 * @param root      Predefined key, e.g. HKEY_LOCAL_MACHINE.
 * @param subKey    Key path below root.
 * @param valueName Value name.
 * @return The number, or std::nullopt when missing or not a DWORD.
 */
std::optional<std::uint32_t> readRegistryDword(HKEY root, const wchar_t* subKey, const wchar_t* valueName)
{
    DWORD value = 0;
    DWORD bytes = sizeof(value);
    if (::RegGetValueW(root, subKey, valueName, RRF_RT_REG_DWORD, nullptr, &value, &bytes) != ERROR_SUCCESS)
    {
        return std::nullopt;
    }
    return value;
}

} // namespace systemlens::providers::windows_os
