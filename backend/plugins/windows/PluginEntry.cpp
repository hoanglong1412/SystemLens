/**
 * @file    PluginEntry.cpp (windows)
 * @brief   Exported entry points of the Windows platform plugin (contract: providers/PluginAbi.h).
 *
 * Every entry point is noexcept and catches everything: an exception must not unwind across the
 * library boundary into the host.
 */
#include "PluginAbi.h"

#if SYSTEMLENS_ENABLE_SAMPLE_API
#include "WindowsHelloProvider.h"
#endif

#include <cstdint>

/**
 * @brief Report the ABI version this plugin was built with.
 * @return kPluginAbiVersion.
 */
std::uint32_t systemlens_plugin_abi_version() noexcept
{
    return systemlens::providers::kPluginAbiVersion;
}

// Without the sample API the hello entry points are not exported at all, so the host sees the
// provider as unavailable (provider-level failure, design D5).
#if SYSTEMLENS_ENABLE_SAMPLE_API

/**
 * @brief [SAMPLE API] Create the Windows hello provider.
 * @return New provider, or nullptr when allocation or construction fails.
 */
systemlens::providers::sample::IHelloProvider* systemlens_create_hello_provider() noexcept
{
    try
    {
        return new systemlens::plugin::WindowsHelloProvider();
    }
    catch (...)
    {
        return nullptr;
    }
}

/**
 * @brief [SAMPLE API] Destroy a provider created by systemlens_create_hello_provider.
 * @param provider Provider to destroy; nullptr is ignored.
 */
void systemlens_destroy_hello_provider(systemlens::providers::sample::IHelloProvider* provider) noexcept
{
    // delete runs in the plugin, so the object is freed by the same allocator that created it.
    delete provider;
}

#endif
