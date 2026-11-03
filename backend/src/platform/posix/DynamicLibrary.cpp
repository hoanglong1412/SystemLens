/**
 * @file    DynamicLibrary.cpp (posix)
 * @brief   POSIX implementation of DynamicLibrary (dlopen / dlsym / dlclose), shared by Linux and macOS.
 */
#include "platform/DynamicLibrary.h"

#include <dlfcn.h>

#include <stdexcept>
#include <string>

namespace systemlens::platform {

namespace {

/**
 * @brief Read and clear the last dl* error.
 * @return dlerror() text, or "unknown error" when none is pending.
 */
std::string lastDlError()
{
    const char* error = dlerror();
    return error != nullptr ? std::string(error) : std::string("unknown error");
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
    // A path containing '/' is opened as is (no LD_LIBRARY_PATH / rpath search).
    // RTLD_NOW reports unresolved symbols here instead of at the first call;
    // RTLD_LOCAL keeps the plugin's symbols out of the global namespace.
    m_handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (m_handle == nullptr)
    {
        throw std::runtime_error("cannot load " + path.string() + ": " + lastDlError());
    }
}

/** @brief Unload the library. */
DynamicLibrary::~DynamicLibrary()
{
    if (m_handle != nullptr)
    {
        dlclose(m_handle);
    }
}

/**
 * @brief Look up an exported symbol.
 * @param name Exported (unmangled) name.
 * @return Address of the symbol, or nullptr when the library does not export it.
 */
void* DynamicLibrary::findSymbol(const char* name) const
{
    return dlsym(m_handle, name);
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
