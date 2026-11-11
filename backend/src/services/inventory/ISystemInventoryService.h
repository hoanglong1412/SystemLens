/**
 * @file    ISystemInventoryService.h
 * @brief   Service interface behind GET /api/v1/services, /api/v1/startup and /api/v1/users, and its result types.
 *
 * Platforms that cannot provide a list answer with available = false and a reason instead of an error:
 * that is a normal situation (macOS, no systemd), not a failure of the backend.
 */
#pragma once

#include "providers/ISystemServiceProvider.h"
#include "providers/IStartupProvider.h"
#include "providers/IUserSessionProvider.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::services::inventory {

/** A list that may be unavailable on this platform. */
template <typename T>
struct Inventory
{
    bool available = true;
    std::optional<std::string> reason; ///< Set when available is false.
    std::vector<T> items;
};

/** Resource usage of the processes of one user. */
struct UserUsage
{
    std::string userName;
    std::uint32_t processCount = 0;
    double cpuPercent = 0.0;
    std::uint64_t memoryBytes = 0;
};

/** Sessions (may be unavailable) and usage per user (always computed from the process list). */
struct UserOverview
{
    Inventory<providers::UserSession> sessions;
    std::vector<UserUsage> users;
};

class ISystemInventoryService
{
public:
    virtual ~ISystemInventoryService() = default;

    /** @return Installed services. */
    [[nodiscard]] virtual Inventory<providers::SystemServiceInfo> getServices() const = 0;

    /** @return Programs started at logon. */
    [[nodiscard]] virtual Inventory<providers::StartupItem> getStartupItems() const = 0;

    /**
     * @brief Logged-on sessions and usage per user.
     * @return Overview.
     * @throws std::runtime_error if no metrics snapshot is available (needed for the usage per user).
     */
    [[nodiscard]] virtual UserOverview getUsers() const = 0;
};

} // namespace systemlens::services::inventory
