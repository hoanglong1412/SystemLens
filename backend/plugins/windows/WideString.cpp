/**
 * @file    WideString.cpp
 * @brief   Implementation of toUtf8().
 */
#include "WideString.h"

#include <windows.h>

namespace systemlens::plugin {

/**
 * @brief Convert a UTF-16 string to UTF-8.
 * @param text UTF-16 text (not necessarily NUL-terminated).
 * @return UTF-8 text; invalid sequences become U+FFFD; empty on conversion failure.
 */
std::string toUtf8(std::wstring_view text)
{
    if (text.empty())
    {
        return {};
    }
    const int length = static_cast<int>(text.size());
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, text.data(), length, nullptr, 0, nullptr, nullptr);
    if (bytes <= 0)
    {
        return {};
    }
    std::string result(static_cast<std::size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), length, result.data(), bytes, nullptr, nullptr);
    return result;
}

} // namespace systemlens::plugin
