/**
 * @file    PlatformProviders.h
 * @brief   The set of providers of the current operating system and the factory that creates it.
 *
 * createPlatformProviders() is declared here once and defined once per platform
 * (providers/windows, providers/linux, providers/macos); CMake compiles exactly one of them.
 * Supporting a new operating system means adding a directory, not touching shared code.
 */
#pragma once

#include "providers/ICpuProvider.h"
#include "providers/IDiskProvider.h"
#include "providers/IGpuProvider.h"
#include "providers/IMemoryProvider.h"
#include "providers/INetworkProvider.h"
#include "providers/IProcessProvider.h"
#include "providers/ISystemInfoProvider.h"
#include "providers/ISystemServiceProvider.h"
#include "providers/IStartupProvider.h"
#include "providers/IUserSessionProvider.h"

#include <memory>

namespace systemlens::providers {

/** One provider per data domain; none of the pointers is null. */
struct PlatformProviders
{
    std::shared_ptr<ISystemInfoProvider> systemInfo;
    std::shared_ptr<ICpuProvider> cpu;
    std::shared_ptr<IMemoryProvider> memory;
    std::shared_ptr<IDiskProvider> disk;
    std::shared_ptr<INetworkProvider> network;
    std::shared_ptr<IGpuProvider> gpu;
    std::shared_ptr<IProcessProvider> process;
    std::shared_ptr<ISystemServiceProvider> services;
    std::shared_ptr<IStartupProvider> startup;
    std::shared_ptr<IUserSessionProvider> sessions;
};

PlatformProviders createPlatformProviders();

} // namespace systemlens::providers
