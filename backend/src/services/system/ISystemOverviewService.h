/**
 * @file    ISystemOverviewService.h
 * @brief   Service interface behind GET /api/v1/system/overview, and its result type.
 */
#pragma once

#include "providers/ISystemInfoProvider.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace systemlens::services::system {

/** Process using the most memory. */
struct TopMemoryProcess
{
    std::string name;
    std::uint64_t memoryBytes = 0;
};

/** What the app shell shows: machine identity, uptime and quick stats. */
struct SystemOverview
{
    providers::SystemIdentity machine;
    std::chrono::system_clock::time_point bootTime;
    std::chrono::seconds uptime{};
    std::chrono::milliseconds sampleInterval{};
    std::uint32_t processCount = 0;
    std::optional<std::uint32_t> serviceCount;
    double cpuPercent = 0.0;
    double memoryPercent = 0.0;
    std::optional<double> diskPercent; ///< Busiest disk; nullopt when no disk reports activity.
    double sendBitsPerSecond = 0.0;    ///< All adapters together.
    double receiveBitsPerSecond = 0.0; ///< All adapters together.
    std::optional<TopMemoryProcess> topMemoryProcess;
};

class ISystemOverviewService
{
public:
    virtual ~ISystemOverviewService() = default;

    /**
     * @brief Get the current overview.
     * @return Overview built from the latest metrics snapshot.
     * @throws std::runtime_error if no snapshot is available.
     */
    [[nodiscard]] virtual SystemOverview getOverview() const = 0;
};

} // namespace systemlens::services::system
