/**
 * @file    InventoryController.h
 * @brief   Controller for GET /api/v1/services, /api/v1/startup and /api/v1/users
 *          (contract: api/openapi.yaml, getServices, getStartupItems and getUsers).
 */
#pragma once

#include "api/IController.h"
#include "services/inventory/ISystemInventoryService.h"

#include <memory>

namespace systemlens::api::inventory {

class InventoryController final : public IController
{
public:
    /** Paths served by this controller. */
    static constexpr const char* kServicesRoute = "/api/v1/services";
    static constexpr const char* kStartupRoute = "/api/v1/startup";
    static constexpr const char* kUsersRoute = "/api/v1/users";

    explicit InventoryController(std::shared_ptr<const services::inventory::ISystemInventoryService> service);
    void registerRoutes(IRouteRegistry& registry) override;

private:
    Response handleGetServices(const Request& request) const;
    Response handleGetStartup(const Request& request) const;
    Response handleGetUsers(const Request& request) const;

    std::shared_ptr<const services::inventory::ISystemInventoryService> m_service;
};

} // namespace systemlens::api::inventory
