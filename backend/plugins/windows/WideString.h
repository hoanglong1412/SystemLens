/**
 * @file    WideString.h
 * @brief   UTF-16 (Win32 wide strings) → UTF-8 conversion shared by the Windows providers.
 */
#pragma once

#include <string>
#include <string_view>

namespace systemlens::plugin {

std::string toUtf8(std::wstring_view text);

} // namespace systemlens::plugin
