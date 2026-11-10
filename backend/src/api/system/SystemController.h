/**
 * @file    SystemController.h
 * @brief   Controller for GET /api/v1/system/overview (contract: api/openapi.yaml, getSystemOverview).
 */
#pragma once

#include "api/IController.h"
#include "services/system/ISystemOverviewService.h"

#include <memory>

namespace systemlens::api::system {

class SystemController final : public IController
{
public:
    /** Path served by this controller. */
    static constexpr const char* kOverviewRoute = "/api/v1/system/overview";

    explicit SystemController(std::shared_ptr<const services::system::ISystemOverviewService> service);
    void registerRoutes(IRouteRegistry& registry) override;

private:
    Response handleGetOverview(const Request& request) const;

    std::shared_ptr<const services::system::ISystemOverviewService> m_service;
};

} // namespace systemlens::api::system
