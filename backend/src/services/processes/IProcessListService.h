/**
 * @file    IProcessListService.h
 * @brief   Service interface behind GET /api/v1/processes and GET /api/v1/apps/history, and its result types.
 */
#pragma once

#include "services/monitoring/MetricsSnapshot.h"

#include <chrono>
#include <vector>

namespace systemlens::services::processes {

/** Per-application usage since the backend started. */
struct AppHistory
{
    std::chrono::system_clock::time_point since;
    std::vector<monitoring::AppUsage> apps;
};

class IProcessListService
{
public:
    virtual ~IProcessListService() = default;

    /**
     * @brief List every process.
     * @return Processes, highest CPU first.
     * @throws std::runtime_error if no metrics snapshot is available.
     */
    [[nodiscard]] virtual std::vector<monitoring::ProcessMetrics> getProcesses() const = 0;

    /**
     * @brief Usage per application name accumulated since the backend started.
     * @return History, highest CPU time first.
     * @throws std::runtime_error if no metrics snapshot is available.
     */
    [[nodiscard]] virtual AppHistory getAppHistory() const = 0;
};

} // namespace systemlens::services::processes
