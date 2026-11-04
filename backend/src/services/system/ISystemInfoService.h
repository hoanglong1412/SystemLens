/**
 * @file    ISystemInfoService.h
 * @brief   Service interface behind GET /api/v1/system/info.
 */
#pragma once

#include "providers/system/ISystemInfoProvider.h"

namespace systemlens::services::system {

class ISystemInfoService
{
public:
    virtual ~ISystemInfoService() = default;

    /**
     * @brief Get the identity of the machine.
     * @return Identity reported by the platform.
     * @throws services::FeatureUnavailableError when the platform has no system info provider.
     */
    virtual providers::system::SystemIdentity getIdentity() const = 0;
};

} // namespace systemlens::services::system
