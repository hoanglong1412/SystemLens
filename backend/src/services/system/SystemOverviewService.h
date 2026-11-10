/**
 * @file    SystemOverviewService.h
 * @brief   Default ISystemOverviewService: derives the overview from the latest MetricsSnapshot.
 */
#pragma once

#include "services/monitoring/IMetricsSource.h"
#include "services/system/ISystemOverviewService.h"

#include <memory>

namespace systemlens::services::system {

class SystemOverviewService final : public ISystemOverviewService
{
public:
    explicit SystemOverviewService(std::shared_ptr<const monitoring::IMetricsSource> metrics);
    [[nodiscard]] SystemOverview getOverview() const override;

private:
    std::shared_ptr<const monitoring::IMetricsSource> m_metrics;
};

} // namespace systemlens::services::system
