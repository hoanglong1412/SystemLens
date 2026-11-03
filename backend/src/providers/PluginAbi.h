/**
 * @file    PluginAbi.h
 * @brief   Binary contract between the backend (host) and the platform plugin
 *          (systemlens_platform): ABI version, entry point names and signatures, export macro.
 *
 * The plugin exports C++ provider interfaces through extern "C" create/destroy functions.
 * Passing std::string and vtables across the library boundary is only valid when host and plugin
 * share the compiler, standard library and CRT. This is guaranteed by building both in the same
 * CMake tree and configuration with the dynamic CRT (/MD, /MDd); it is NOT checked at runtime.
 * kPluginAbiVersion only catches interface changes, not compiler or CRT differences.
 *
 * Rules:
 *  - Bump kPluginAbiVersion on ANY change to a provider interface or to an entry point below.
 *  - Entry points are noexcept: an exception must never unwind across the library boundary.
 *  - Objects created by the plugin are destroyed by the plugin (destroy_*), never by delete in the host.
 */
#pragma once

// Relative include: the plugin only has src/providers/ on its include path, the host has src/.
#include "sample/IHelloProvider.h"

#include <cstdint>

// Symbols exported by the plugin. The host includes this header without SYSTEMLENS_PLUGIN_BUILD,
// so on its side the macro only gives the declarations C linkage.
#if defined(SYSTEMLENS_PLUGIN_BUILD)
#if defined(_WIN32)
#define SYSTEMLENS_PLUGIN_EXPORT __declspec(dllexport)
#else
#define SYSTEMLENS_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif
#else
#define SYSTEMLENS_PLUGIN_EXPORT
#endif

namespace systemlens::providers {

/** ABI version of the provider interfaces and entry points declared here. */
inline constexpr std::uint32_t kPluginAbiVersion = 1;

/** Entry point names, used by the host to look up the symbols the plugin exports. */
inline constexpr const char* kAbiVersionEntryPoint = "systemlens_plugin_abi_version";
inline constexpr const char* kCreateHelloProviderEntryPoint = "systemlens_create_hello_provider";
inline constexpr const char* kDestroyHelloProviderEntryPoint = "systemlens_destroy_hello_provider";

/** Entry point signatures, used by the host to cast looked-up symbols. */
using AbiVersionFn = std::uint32_t (*)() noexcept;
using CreateHelloProviderFn = sample::IHelloProvider* (*)() noexcept;
using DestroyHelloProviderFn = void (*)(sample::IHelloProvider*) noexcept;

} // namespace systemlens::providers

extern "C" {

/**
 * @brief Report the ABI version the plugin was built with.
 * @return kPluginAbiVersion of the plugin build.
 */
SYSTEMLENS_PLUGIN_EXPORT std::uint32_t systemlens_plugin_abi_version() noexcept;

/**
 * @brief [SAMPLE API] Create the hello provider of this platform.
 * @return New provider owned by the caller (release with systemlens_destroy_hello_provider),
 *         or nullptr on failure.
 */
SYSTEMLENS_PLUGIN_EXPORT systemlens::providers::sample::IHelloProvider* systemlens_create_hello_provider() noexcept;

/**
 * @brief [SAMPLE API] Destroy a provider created by systemlens_create_hello_provider.
 * @param provider Provider to destroy; nullptr is ignored.
 */
SYSTEMLENS_PLUGIN_EXPORT void systemlens_destroy_hello_provider(systemlens::providers::sample::IHelloProvider* provider) noexcept;

} // extern "C"
