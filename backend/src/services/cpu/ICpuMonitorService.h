/**
 * @file    ICpuMonitorService.h
 * @brief   Service interface behind GET /api/v1/cpu/info and GET /api/v1/cpu/metrics.
 *          Interface so controllers can be tested with a fake.
 */
#pragma once

#include "providers/cpu/ICpuProvider.h"
#include "services/cpu/CpuMetrics.h"

namespace systemlens::services::cpu {

class ICpuMonitorService
{
public:
    virtual ~ICpuMonitorService() = default;

    /**
     * @brief Get the static processor facts.
     * @return Static info read once at start.
     * @throws services::FeatureUnavailableError when the platform has no CPU provider.
     * @throws services::NotReadyError when the service has not been started yet.
     */
    virtual providers::cpu::CpuStaticInfo getInfo() const = 0;

    /**
     * @brief Get the metrics of the last interval and the history.
     * @return Copy of the latest metrics.
     * @throws services::FeatureUnavailableError when the platform has no CPU provider.
     * @throws services::NotReadyError before the first interval has been measured.
     */
    virtual CpuMetrics getMetrics() const = 0;
};

} // namespace systemlens::services::cpu
