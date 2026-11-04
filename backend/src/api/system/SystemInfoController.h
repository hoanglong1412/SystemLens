/**
 * @file    SystemInfoController.h
 * @brief   Controller for GET /api/v1/system/info (contract: api/openapi.yaml).
 */
#pragma once

#include "api/IController.h"
#include "services/system/ISystemInfoService.h"

#include <memory>

namespace systemlens::api::system {

class SystemInfoController final : public IController
{
public:
    /** Path served by this controller. */
    static constexpr const char* kRoute = "/api/v1/system/info";

    explicit SystemInfoController(std::shared_ptr<const services::system::ISystemInfoService> service);
    void registerRoutes(IRouteRegistry& registry) override;

private:
    Response handleGetInfo(const Request& request) const;

    std::shared_ptr<const services::system::ISystemInfoService> m_service;
};

} // namespace systemlens::api::system
