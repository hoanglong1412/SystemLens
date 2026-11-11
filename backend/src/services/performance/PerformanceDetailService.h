/**
 * @file    PerformanceDetailService.h
 * @brief   Default IPerformanceDetailService: derives its results from the latest MetricsSnapshot.
 */
#pragma once

#include "services/monitoring/IMetricsSource.h"
#include "services/performance/IPerformanceDetailService.h"

#include <memory>

namespace systemlens::services::performance {

class PerformanceDetailService final : public IPerformanceDetailService
{
public:
    explicit PerformanceDetailService(std::shared_ptr<const monitoring::IMetricsSource> metrics);
    [[nodiscard]] std::vector<CoreUsage> getCores() const override;
    [[nodiscard]] MemoryDetail getMemory() const override;
    [[nodiscard]] std::vector<monitoring::DiskMetrics> getDisks() const override;
    [[nodiscard]] std::vector<monitoring::NetworkMetrics> getNetworks() const override;
    [[nodiscard]] std::vector<monitoring::GpuMetrics> getGpus() const override;

private:
    std::shared_ptr<const monitoring::IMetricsSource> m_metrics;
};

} // namespace systemlens::services::performance
