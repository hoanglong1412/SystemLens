/**
 * @file    WindowsPlatformProviders.cpp
 * @brief   Windows definition of createPlatformProviders().
 */
#include "providers/PlatformProviders.h"

#include "providers/windows/WindowsCpuProvider.h"
#include "providers/windows/WindowsDiskProvider.h"
#include "providers/windows/WindowsGpuProvider.h"
#include "providers/windows/WindowsMemoryProvider.h"
#include "providers/windows/WindowsNetworkProvider.h"
#include "providers/windows/WindowsProcessProvider.h"
#include "providers/windows/WindowsServiceProvider.h"
#include "providers/windows/WindowsStartupProvider.h"
#include "providers/windows/WindowsUserSessionProvider.h"
#include "providers/windows/WindowsSystemInfoProvider.h"

namespace systemlens::providers {

/**
 * @brief Create the Windows providers.
 * @return One provider per domain.
 * @throws ProviderError if a provider cannot initialize.
 */
PlatformProviders createPlatformProviders()
{
    return PlatformProviders{
        .systemInfo = std::make_shared<windows_os::WindowsSystemInfoProvider>(),
        .cpu = std::make_shared<windows_os::WindowsCpuProvider>(),
        .memory = std::make_shared<windows_os::WindowsMemoryProvider>(),
        .disk = std::make_shared<windows_os::WindowsDiskProvider>(),
        .network = std::make_shared<windows_os::WindowsNetworkProvider>(),
        .gpu = std::make_shared<windows_os::WindowsGpuProvider>(),
        .process = std::make_shared<windows_os::WindowsProcessProvider>(),
        .services = std::make_shared<windows_os::WindowsServiceProvider>(),
        .startup = std::make_shared<windows_os::WindowsStartupProvider>(),
        .sessions = std::make_shared<windows_os::WindowsUserSessionProvider>(),
    };
}

} // namespace systemlens::providers
