/**
 * @file    ProcessController.h
 * @brief   Controller for GET /api/v1/processes and GET /api/v1/apps/history
 *          (contract: api/openapi.yaml, getProcesses and getAppHistory).
 */
#pragma once

#include "api/IController.h"
#include "services/processes/IProcessListService.h"

#include <memory>

namespace systemlens::api::processes {

class ProcessController final : public IController
{
public:
    /** Paths served by this controller. */
    static constexpr const char* kProcessesRoute = "/api/v1/processes";
    static constexpr const char* kAppHistoryRoute = "/api/v1/apps/history";

    explicit ProcessController(std::shared_ptr<const services::processes::IProcessListService> service);
    void registerRoutes(IRouteRegistry& registry) override;

private:
    Response handleGetProcesses(const Request& request) const;
    Response handleGetAppHistory(const Request& request) const;

    std::shared_ptr<const services::processes::IProcessListService> m_service;
};

} // namespace systemlens::api::processes
