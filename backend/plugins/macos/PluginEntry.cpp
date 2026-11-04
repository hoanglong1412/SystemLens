/**
 * @file    PluginEntry.cpp (macos)
 * @brief   Exported entry points of the macOS platform plugin (contract: providers/PluginAbi.h).
 *          Written but not built in this change (no macOS build machine).
 *
 * Every entry point is noexcept and catches everything: an exception must not unwind across the
 * library boundary into the host.
 */
#include "PluginAbi.h"

#if SYSTEMLENS_ENABLE_SAMPLE_API
#include "MacHelloProvider.h"
#endif

#include <cstdint>

// No CPU or system info provider on macOS yet: their entry points are not exported, so the host sees
// the providers as unavailable and the /api/v1/cpu/* and /api/v1/system/* endpoints answer 501.

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
 * @brief [SAMPLE API] Create the macOS hello provider.
 * @return New provider, or nullptr when allocation or construction fails.
 */
systemlens::providers::sample::IHelloProvider* systemlens_create_hello_provider() noexcept
{
    try
    {
        return new systemlens::plugin::MacHelloProvider();
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
