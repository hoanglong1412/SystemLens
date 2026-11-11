/**
 * @file    SystemInventoryService.cpp
 * @brief   Implementation of SystemInventoryService.
 */
#include "services/inventory/SystemInventoryService.h"

#include "providers/ProviderError.h"

#include <algorithm>
#include <map>
#include <stdexcept>

namespace systemlens::services::inventory {

namespace {

/** Group name of the processes whose owner the backend cannot read. */
constexpr const char* kUnknownUser = "(unknown)";

/**
 * @brief Run a provider call and turn a provider failure into "not available".
 * @param read Callable returning a vector of items; may throw providers::ProviderError.
 * @return The items, or an unavailable inventory carrying the error text.
 */
template <typename Item, typename Read>
Inventory<Item> collectInventory(Read&& read)
{
    Inventory<Item> inventory;
    try
    {
        inventory.items = read();
    }
    catch (const providers::ProviderError& e)
    {
        inventory.available = false;
        inventory.reason = e.what();
    }
    return inventory;
}

/**
 * @brief Check a dependency passed to the constructor.
 * @param pointer Dependency.
 * @param name    Its name for the error message.
 * @throws std::invalid_argument if pointer is null.
 */
void requireNotNull(const void* pointer, const char* name)
{
    if (pointer == nullptr)
    {
        throw std::invalid_argument(std::string("SystemInventoryService: ") + name + " must not be null");
    }
}

} // namespace

/**
 * @brief Create the service.
 * @param services Provider of the service list.
 * @param startup  Provider of the startup items.
 * @param sessions Provider of the logon sessions.
 * @param metrics  Source of metrics snapshots (processes per user).
 * @throws std::invalid_argument if a dependency is null.
 */
SystemInventoryService::SystemInventoryService(std::shared_ptr<const providers::ISystemServiceProvider> services,
                                               std::shared_ptr<const providers::IStartupProvider> startup,
                                               std::shared_ptr<const providers::IUserSessionProvider> sessions,
                                               std::shared_ptr<const monitoring::IMetricsSource> metrics)
    : m_services(std::move(services))
    , m_startup(std::move(startup))
    , m_sessions(std::move(sessions))
    , m_metrics(std::move(metrics))
{
    requireNotNull(m_services.get(), "service provider");
    requireNotNull(m_startup.get(), "startup provider");
    requireNotNull(m_sessions.get(), "session provider");
    requireNotNull(m_metrics.get(), "metrics source");
}

/**
 * @brief List the services, sorted by name.
 * @return Services, or an unavailable inventory when the platform cannot list them.
 */
Inventory<providers::SystemServiceInfo> SystemInventoryService::getServices() const
{
    auto inventory = collectInventory<providers::SystemServiceInfo>([this] { return m_services->readServices(); });
    std::ranges::sort(inventory.items, {}, &providers::SystemServiceInfo::name);
    return inventory;
}

/**
 * @brief List the startup items, sorted by name.
 * @return Items, or an unavailable inventory when the platform cannot list them.
 */
Inventory<providers::StartupItem> SystemInventoryService::getStartupItems() const
{
    auto inventory = collectInventory<providers::StartupItem>([this] { return m_startup->readStartupItems(); });
    std::ranges::sort(inventory.items, {}, &providers::StartupItem::name);
    return inventory;
}

/**
 * @brief Logged-on sessions plus CPU / memory totals per process owner.
 * @return Overview; users are sorted by CPU, then memory, highest first.
 * @throws std::runtime_error if the metrics source has no valid snapshot.
 */
UserOverview SystemInventoryService::getUsers() const
{
    UserOverview overview;
    overview.sessions = collectInventory<providers::UserSession>([this] { return m_sessions->readSessions(); });

    std::map<std::string, UserUsage> perUser;
    for (const monitoring::ProcessMetrics& process : m_metrics->latest()->processes)
    {
        const std::string& owner = process.userName.empty() ? std::string(kUnknownUser) : process.userName;
        UserUsage& usage = perUser.try_emplace(owner, UserUsage{.userName = owner}).first->second;
        ++usage.processCount;
        usage.cpuPercent += process.cpuPercent;
        usage.memoryBytes += process.memoryBytes;
    }
    overview.users.reserve(perUser.size());
    for (auto& [name, usage] : perUser)
    {
        overview.users.push_back(std::move(usage));
    }
    std::ranges::sort(overview.users, [](const UserUsage& a, const UserUsage& b) {
        return a.cpuPercent != b.cpuPercent ? a.cpuPercent > b.cpuPercent : a.memoryBytes > b.memoryBytes;
    });
    return overview;
}

} // namespace systemlens::services::inventory
