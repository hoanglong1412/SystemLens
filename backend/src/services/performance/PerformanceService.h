/**
 * @file    PerformanceService.h
 * @brief   Default IPerformanceService: derives its results from the latest MetricsSnapshot.
 */
#pragma once

#include "services/monitoring/IMetricsSource.h"
#include "services/performance/IPerformanceService.h"

#include <memory>

namespace systemlens::services::performance {

class PerformanceService final : public IPerformanceService
{
public:
    explicit PerformanceService(std::shared_ptr<const monitoring::IMetricsSource> metrics);
    [[nodiscard]] std::vector<ResourceSummary> getResources() const override;
    [[nodiscard]] CpuPerformance getCpuPerformance() const override;

private:
    std::shared_ptr<const monitoring::IMetricsSource> m_metrics;
};

} // namespace systemlens::services::performance
