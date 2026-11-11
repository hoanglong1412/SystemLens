/**
 * @file    PerformanceDetailController.h
 * @brief   Controller for the per-resource performance endpoints (contract: api/openapi.yaml):
 *          getCpuCores, getMemoryPerformance, getDiskPerformance, getNetworkPerformance, getGpuPerformance.
 */
#pragma once

#include "api/IController.h"
#include "services/performance/IPerformanceDetailService.h"

#include <memory>

namespace systemlens::api::performance {

class PerformanceDetailController final : public IController
{
public:
    /** Paths served by this controller. */
    static constexpr const char* kCoresRoute = "/api/v1/performance/cpu/cores";
    static constexpr const char* kMemoryRoute = "/api/v1/performance/memory";
    static constexpr const char* kDisksRoute = "/api/v1/performance/disks";
    static constexpr const char* kNetworksRoute = "/api/v1/performance/networks";
    static constexpr const char* kGpusRoute = "/api/v1/performance/gpus";

    explicit PerformanceDetailController(std::shared_ptr<const services::performance::IPerformanceDetailService> service);
    void registerRoutes(IRouteRegistry& registry) override;

private:
    std::shared_ptr<const services::performance::IPerformanceDetailService> m_service;
};

} // namespace systemlens::api::performance
