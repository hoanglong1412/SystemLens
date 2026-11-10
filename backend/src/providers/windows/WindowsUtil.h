/**
 * @file    WindowsUtil.h
 * @brief   Helpers shared by the Windows providers: UTF-8 conversion, RAII handles, registry reads.
 */
#pragma once

#include <windows.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace systemlens::providers::windows_os {

/** Closes a kernel HANDLE; used as the deleter of UniqueHandle. */
struct HandleCloser
{
    void operator()(HANDLE handle) const noexcept;
};

/** Owning kernel handle. Empty (null) when the open failed. */
using UniqueHandle = std::unique_ptr<void, HandleCloser>;

std::string toUtf8(std::wstring_view text);
std::chrono::duration<double> fromHundredNanoseconds(std::int64_t ticks);
UniqueHandle openForQuery(const std::wstring& path);
std::optional<std::wstring> readRegistryString(HKEY root, const wchar_t* subKey, const wchar_t* valueName);
std::optional<std::uint32_t> readRegistryDword(HKEY root, const wchar_t* subKey, const wchar_t* valueName);

} // namespace systemlens::providers::windows_os
