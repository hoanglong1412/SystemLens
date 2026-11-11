/**
 * @file    ProcessListService.cpp
 * @brief   Implementation of ProcessListService.
 */
#include "services/processes/ProcessListService.h"

#include <stdexcept>

namespace systemlens::services::processes {

/**
 * @brief Create the service.
 * @param metrics Source of metrics snapshots. Must not be null.
 * @throws std::invalid_argument if metrics is null.
 */
ProcessListService::ProcessListService(std::shared_ptr<const monitoring::IMetricsSource> metrics)
    : m_metrics(std::move(metrics))
{
    if (!m_metrics)
    {
        throw std::invalid_argument("ProcessListService: metrics source must not be null");
    }
}

/**
 * @brief Every process of the latest snapshot.
 * @return Processes, highest CPU first.
 * @throws std::runtime_error if the metrics source has no valid snapshot.
 */
std::vector<monitoring::ProcessMetrics> ProcessListService::getProcesses() const
{
    return m_metrics->latest()->processes;
}

/**
 * @brief The per-application totals of the latest snapshot.
 * @return History since the collector started.
 * @throws std::runtime_error if the metrics source has no valid snapshot.
 */
AppHistory ProcessListService::getAppHistory() const
{
    const auto snapshot = m_metrics->latest();
    return AppHistory{.since = snapshot->appHistorySince, .apps = snapshot->appHistory};
}

} // namespace systemlens::services::processes
