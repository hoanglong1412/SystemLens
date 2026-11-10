/**
 * @file    PerformanceController.h
 * @brief   Controller for GET /api/v1/performance/resources and GET /api/v1/performance/cpu
 *          (contract: api/openapi.yaml, getPerformanceResources and getCpuPerformance).
 */
#pragma once

#include "api/IController.h"
#include "services/performance/IPerformanceService.h"

#include <memory>

namespace systemlens::api::performance {

class PerformanceController final : public IController
{
public:
    /** Paths served by this controller. */
    static constexpr const char* kResourcesRoute = "/api/v1/performance/resources";
    static constexpr const char* kCpuRoute = "/api/v1/performance/cpu";

    explicit PerformanceController(std::shared_ptr<const services::performance::IPerformanceService> service);
    void registerRoutes(IRouteRegistry& registry) override;

private:
    Response handleGetResources(const Request& request) const;
    Response handleGetCpu(const Request& request) const;

    std::shared_ptr<const services::performance::IPerformanceService> m_service;
};

} // namespace systemlens::api::performance
