/**
 * @file    DynamicLibrary.cpp (windows)
 * @brief   Windows implementation of DynamicLibrary (LoadLibraryExW / GetProcAddress / FreeLibrary).
 */
#include "platform/DynamicLibrary.h"

#include <windows.h>

#include <stdexcept>
#include <string>

namespace systemlens::platform {

namespace {

/**
 * @brief Describe a Win32 error code.
 * @param code Value returned by GetLastError().
 * @return System message followed by the numeric code.
 */
std::string describeError(DWORD code)
{
    char* buffer = nullptr;
    const DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code,
        0, reinterpret_cast<char*>(&buffer), 0, nullptr);
    std::string message = length > 0 ? std::string(buffer, length) : std::string("unknown error");
    LocalFree(buffer);
    // FormatMessage ends the text with "\r\n", which would break the single-line log.
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r' || message.back() == ' '))
    {
        message.pop_back();
    }
    return message + " (error " + std::to_string(code) + ")";
}

} // namespace

/**
 * @brief Load the library at an absolute path.
 * @param path Absolute path of the library file.
 * @throws std::runtime_error when the library cannot be loaded.
 */
DynamicLibrary::DynamicLibrary(const std::filesystem::path& path)
    : m_path(path)
{
    // The two search flags restrict the DLL's own dependencies to its directory and System32,
    // never the current directory or PATH (DLL planting). They require an absolute path.
    HMODULE module = LoadLibraryExW(path.c_str(), nullptr,
                                    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (module == nullptr)
    {
        throw std::runtime_error("cannot load " + path.string() + ": " + describeError(GetLastError()));
    }
    m_handle = module;
}

/** @brief Unload the library. */
DynamicLibrary::~DynamicLibrary()
{
    if (m_handle != nullptr)
    {
        FreeLibrary(static_cast<HMODULE>(m_handle));
    }
}

/**
 * @brief Look up an exported symbol.
 * @param name Exported (unmangled) name.
 * @return Address of the symbol, or nullptr when the library does not export it.
 */
void* DynamicLibrary::findSymbol(const char* name) const
{
    // FARPROC -> void*: GetProcAddress returns a function pointer typed as FARPROC; the caller casts
    // it back to the real signature.
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(m_handle), name));
}

/**
 * @brief Path the library was loaded from.
 * @return Absolute path passed to the constructor.
 */
const std::filesystem::path& DynamicLibrary::path() const noexcept
{
    return m_path;
}

} // namespace systemlens::platform
