/**
 * @file    CpuController.h
 * @brief   Controller for GET /api/v1/cpu/info and GET /api/v1/cpu/metrics (contract: api/openapi.yaml).
 *
 * Status mapping (design D5): FeatureUnavailableError → 501, NotReadyError → 503.
 */
#pragma once

#include "api/IController.h"
#include "services/cpu/ICpuMonitorService.h"

#include <memory>

namespace systemlens::api::cpu {

class CpuController final : public IController
{
public:
    /** Paths served by this controller. */
    static constexpr const char* kInfoRoute = "/api/v1/cpu/info";
    static constexpr const char* kMetricsRoute = "/api/v1/cpu/metrics";

    explicit CpuController(std::shared_ptr<const services::cpu::ICpuMonitorService> service);
    void registerRoutes(IRouteRegistry& registry) override;

private:
    Response handleGetInfo(const Request& request) const;
    Response handleGetMetrics(const Request& request) const;

    std::shared_ptr<const services::cpu::ICpuMonitorService> m_service;
};

} // namespace systemlens::api::cpu
