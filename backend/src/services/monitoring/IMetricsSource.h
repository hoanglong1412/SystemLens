/**
 * @file    IMetricsSource.h
 * @brief   Interface through which services read the latest MetricsSnapshot.
 *
 * Implemented by MetricsCollector; an interface so services can be tested with canned snapshots.
 */
#pragma once

#include "services/monitoring/MetricsSnapshot.h"

#include <memory>

namespace systemlens::services::monitoring {

class IMetricsSource
{
public:
    virtual ~IMetricsSource() = default;

    /**
     * @brief Get the result of the most recent sampling tick.
     * @return Shared immutable snapshot, never null. Safe to call from any thread.
     * @throws std::runtime_error if the most recent tick failed (the message says why).
     */
    [[nodiscard]] virtual std::shared_ptr<const MetricsSnapshot> latest() const = 0;
};

} // namespace systemlens::services::monitoring
