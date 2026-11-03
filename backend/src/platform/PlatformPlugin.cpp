/**
 * @file    PlatformPlugin.cpp
 * @brief   Implementation of PlatformPlugin.
 */
#include "platform/PlatformPlugin.h"

#include "providers/PluginAbi.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace systemlens::platform {

namespace {

/**
 * @brief Look up an entry point and cast it to its signature.
 * @tparam Fn Function pointer type of the entry point.
 * @param library Loaded library.
 * @param name Exported name.
 * @return The entry point, or nullptr when it is not exported.
 */
template <typename Fn>
Fn findEntryPoint(const DynamicLibrary& library, const char* name)
{
    // void* -> function pointer is conditionally supported by the standard but is exactly what
    // GetProcAddress/dlsym require on every supported platform.
    return reinterpret_cast<Fn>(library.findSymbol(name));
}

/**
 * @brief Load the library after checking that the file exists.
 * @param libraryPath Absolute path of the platform library.
 * @return Loaded library.
 * @throws std::runtime_error naming the expected path when the file is missing or cannot be loaded.
 */
std::shared_ptr<DynamicLibrary> loadLibrary(const std::filesystem::path& libraryPath)
{
    // Checked first so that the error names the expected path instead of a bare OS error.
    std::error_code error;
    if (!std::filesystem::is_regular_file(libraryPath, error))
    {
        throw std::runtime_error("platform library not found: " + libraryPath.string());
    }
    return std::make_shared<DynamicLibrary>(libraryPath);
}

/**
 * @brief Write the provider-level warning to stderr.
 * @param provider Provider name.
 * @param reason Why it is unavailable.
 */
void warnProviderUnavailable(const char* provider, const std::string& reason)
{
    std::cerr << "[PlatformPlugin] WARNING: " << provider << " provider unavailable: " << reason << std::endl;
}

} // namespace

/**
 * @brief Load the platform library and check its ABI version.
 * @param libraryPath Absolute path of the platform library.
 * @throws std::runtime_error when the library is missing, cannot be loaded, does not export the
 *         ABI version entry point, or reports a different ABI version.
 */
PlatformPlugin::PlatformPlugin(const std::filesystem::path& libraryPath)
    : m_library(loadLibrary(libraryPath))
{
    const auto abiVersion = findEntryPoint<providers::AbiVersionFn>(*m_library, providers::kAbiVersionEntryPoint);
    if (abiVersion == nullptr)
    {
        throw std::runtime_error(libraryPath.string() + " does not export " + providers::kAbiVersionEntryPoint);
    }

    const std::uint32_t pluginVersion = abiVersion();
    if (pluginVersion != providers::kPluginAbiVersion)
    {
        throw std::runtime_error(libraryPath.string() + " has ABI version " + std::to_string(pluginVersion) +
                                 ", backend expects " + std::to_string(providers::kPluginAbiVersion));
    }
    std::cerr << "[PlatformPlugin] loaded " << libraryPath.string() << " (ABI " << pluginVersion << ")" << std::endl;
}

/**
 * @brief [SAMPLE API] Create the hello provider exported by the plugin.
 * @return The provider, or an empty pointer (after a warning on stderr) when the plugin does not
 *         export it or fails to create it. Destroying the provider calls the plugin's destroy function.
 */
std::shared_ptr<const providers::sample::IHelloProvider> PlatformPlugin::createHelloProvider() const
{
    constexpr const char* kProviderName = "hello";

    const auto create =
        findEntryPoint<providers::CreateHelloProviderFn>(*m_library, providers::kCreateHelloProviderEntryPoint);
    const auto destroy =
        findEntryPoint<providers::DestroyHelloProviderFn>(*m_library, providers::kDestroyHelloProviderEntryPoint);
    if (create == nullptr || destroy == nullptr)
    {
        warnProviderUnavailable(kProviderName, std::string("missing entry point ") +
                                                   (create == nullptr ? providers::kCreateHelloProviderEntryPoint
                                                                      : providers::kDestroyHelloProviderEntryPoint));
        return nullptr;
    }

    providers::sample::IHelloProvider* provider = create();
    if (provider == nullptr)
    {
        warnProviderUnavailable(kProviderName, std::string(providers::kCreateHelloProviderEntryPoint) +
                                                   " returned no provider");
        return nullptr;
    }

    // The deleter owns a copy of m_library: the library outlives the provider whatever the
    // destruction order of the objects that hold it. shared_ptr passes the deleter the original
    // non-const pointer, so no const_cast is needed.
    return std::shared_ptr<const providers::sample::IHelloProvider>(
        provider, [library = m_library, destroy](providers::sample::IHelloProvider* p) { destroy(p); });
}

} // namespace systemlens::platform
