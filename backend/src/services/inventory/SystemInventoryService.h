/**
 * @file    SystemInventoryService.h
 * @brief   Default ISystemInventoryService: asks the service, startup and session providers on demand and
 *          derives the usage per user from the latest MetricsSnapshot.
 */
#pragma once

#include "services/inventory/ISystemInventoryService.h"
#include "services/monitoring/IMetricsSource.h"

#include <memory>

namespace systemlens::services::inventory {

class SystemInventoryService final : public ISystemInventoryService
{
public:
    SystemInventoryService(std::shared_ptr<const providers::ISystemServiceProvider> services,
                           std::shared_ptr<const providers::IStartupProvider> startup,
                           std::shared_ptr<const providers::IUserSessionProvider> sessions,
                           std::shared_ptr<const monitoring::IMetricsSource> metrics);

    [[nodiscard]] Inventory<providers::SystemServiceInfo> getServices() const override;
    [[nodiscard]] Inventory<providers::StartupItem> getStartupItems() const override;
    [[nodiscard]] UserOverview getUsers() const override;

private:
    std::shared_ptr<const providers::ISystemServiceProvider> m_services;
    std::shared_ptr<const providers::IStartupProvider> m_startup;
    std::shared_ptr<const providers::IUserSessionProvider> m_sessions;
    std::shared_ptr<const monitoring::IMetricsSource> m_metrics;
};

} // namespace systemlens::services::inventory
