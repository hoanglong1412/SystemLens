/**
 * @file    ProcessListService.h
 * @brief   Default IProcessListService: reads the process data of the latest MetricsSnapshot.
 */
#pragma once

#include "services/monitoring/IMetricsSource.h"
#include "services/processes/IProcessListService.h"

#include <memory>

namespace systemlens::services::processes {

class ProcessListService final : public IProcessListService
{
public:
    explicit ProcessListService(std::shared_ptr<const monitoring::IMetricsSource> metrics);
    [[nodiscard]] std::vector<monitoring::ProcessMetrics> getProcesses() const override;
    [[nodiscard]] AppHistory getAppHistory() const override;

private:
    std::shared_ptr<const monitoring::IMetricsSource> m_metrics;
};

} // namespace systemlens::services::processes
