/**
 * @file    SampleHelloService.cpp
 * @brief   [SAMPLE API] Implementation of SampleHelloService.
 */
#include "services/sample/SampleHelloService.h"

#include "services/FeatureUnavailableError.h"

namespace systemlens::services::sample {

/**
 * @brief Create the service.
 * @param provider Platform hello provider. May be empty: the service is still created and
 *                 getGreeting() then reports the feature as unavailable.
 */
SampleHelloService::SampleHelloService(std::shared_ptr<const providers::sample::IHelloProvider> provider)
    : m_provider(std::move(provider))
{
}

/**
 * @brief Get the platform greeting.
 * @return Greeting supplied by the hello provider.
 * @throws services::FeatureUnavailableError when the platform has no hello provider.
 */
std::string SampleHelloService::getGreeting() const
{
    if (!m_provider)
    {
        throw FeatureUnavailableError("the platform library provides no hello provider");
    }
    return m_provider->getGreeting();
}

} // namespace systemlens::services::sample
