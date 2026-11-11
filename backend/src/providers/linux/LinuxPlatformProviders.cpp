/**
 * @file    LinuxPlatformProviders.cpp
 * @brief   Linux definition of createPlatformProviders().
 */
#include "providers/PlatformProviders.h"

#include "providers/linux/LinuxCpuProvider.h"
#include "providers/linux/LinuxDiskProvider.h"
#include "providers/linux/LinuxGpuProvider.h"
#include "providers/linux/LinuxMemoryProvider.h"
#include "providers/linux/LinuxNetworkProvider.h"
#include "providers/linux/LinuxProcessProvider.h"
#include "providers/linux/LinuxServiceProvider.h"
#include "providers/linux/LinuxStartupProvider.h"
#include "providers/linux/LinuxUserSessionProvider.h"
#include "providers/linux/LinuxSystemInfoProvider.h"

namespace systemlens::providers {

/**
 * @brief Create the Linux providers.
 * @return One provider per domain.
 * @throws ProviderError if a provider cannot initialize.
 */
PlatformProviders createPlatformProviders()
{
    return PlatformProviders{
        .systemInfo = std::make_shared<linux_os::LinuxSystemInfoProvider>(),
        .cpu = std::make_shared<linux_os::LinuxCpuProvider>(),
        .memory = std::make_shared<linux_os::LinuxMemoryProvider>(),
        .disk = std::make_shared<linux_os::LinuxDiskProvider>(),
        .network = std::make_shared<linux_os::LinuxNetworkProvider>(),
        .gpu = std::make_shared<linux_os::LinuxGpuProvider>(),
        .process = std::make_shared<linux_os::LinuxProcessProvider>(),
        .services = std::make_shared<linux_os::LinuxServiceProvider>(),
        .startup = std::make_shared<linux_os::LinuxStartupProvider>(),
        .sessions = std::make_shared<linux_os::LinuxUserSessionProvider>(),
    };
}

} // namespace systemlens::providers
