/**
 * @file    SystemInfoService.cpp
 * @brief   Implementation of SystemInfoService.
 */
#include "services/system/SystemInfoService.h"

#include "services/FeatureUnavailableError.h"

namespace systemlens::services::system {

/**
 * @brief Create the service.
 * @param provider System info provider of the platform; empty when the platform has none.
 */
SystemInfoService::SystemInfoService(std::shared_ptr<const providers::system::ISystemInfoProvider> provider)
    : m_provider(std::move(provider))
{
}

/**
 * @brief Get the identity of the machine from the provider.
 * @return Identity.
 * @throws FeatureUnavailableError when there is no provider; std::exception from the provider.
 */
providers::system::SystemIdentity SystemInfoService::getIdentity() const
{
    if (!m_provider)
    {
        throw FeatureUnavailableError("System information is not available on this platform");
    }
    return m_provider->getIdentity();
}

} // namespace systemlens::services::system
