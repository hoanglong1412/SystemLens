/**
 * @file    SystemInfoService.h
 * @brief   Default ISystemInfoService: forwards to the platform's system info provider.
 */
#pragma once

#include "services/system/ISystemInfoService.h"

#include <memory>

namespace systemlens::services::system {

class SystemInfoService final : public ISystemInfoService
{
public:
    explicit SystemInfoService(std::shared_ptr<const providers::system::ISystemInfoProvider> provider);

    providers::system::SystemIdentity getIdentity() const override;

private:
    std::shared_ptr<const providers::system::ISystemInfoProvider> m_provider; ///< Empty when unavailable.
};

} // namespace systemlens::services::system
