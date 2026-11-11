/**
 * @file    InventoryController.cpp
 * @brief   Implementation of InventoryController.
 */
#include "api/inventory/InventoryController.h"

#include "api/ApiResponse.h"
#include "api/JsonHelpers.h"

#include <stdexcept>

namespace systemlens::api::inventory {

namespace {

using nlohmann::json;

/**
 * @brief Name a service state as in the ServiceEntry.state enum of the contract.
 * @param state Service state.
 * @return "running", "stopped", "starting", "stopping", "paused" or "other".
 */
const char* toString(providers::ServiceState state)
{
    switch (state)
    {
    case providers::ServiceState::Running:
        return "running";
    case providers::ServiceState::Stopped:
        return "stopped";
    case providers::ServiceState::Starting:
        return "starting";
    case providers::ServiceState::Stopping:
        return "stopping";
    case providers::ServiceState::Paused:
        return "paused";
    case providers::ServiceState::Other:
        break;
    }
    return "other";
}

/**
 * @brief Name a start type as in the ServiceEntry.startType enum of the contract.
 * @param type Start type.
 * @return "automatic", "manual", "disabled", "static" or "other".
 */
const char* toString(providers::ServiceStartType type)
{
    switch (type)
    {
    case providers::ServiceStartType::Automatic:
        return "automatic";
    case providers::ServiceStartType::Manual:
        return "manual";
    case providers::ServiceStartType::Disabled:
        return "disabled";
    case providers::ServiceStartType::Static:
        return "static";
    case providers::ServiceStartType::Other:
        break;
    }
    return "other";
}

/**
 * @brief Build the response skeleton shared by the three endpoints.
 * @param available Whether the list could be read.
 * @param reason    Why not, when unavailable.
 * @return {"available": ..., "reason": ...}.
 */
json availabilityJson(bool available, const std::optional<std::string>& reason)
{
    return {{"available", available}, {"reason", nullable(reason)}};
}

} // namespace

/**
 * @brief Create the controller.
 * @param service Provides the data. Must not be null.
 * @throws std::invalid_argument if service is null.
 */
InventoryController::InventoryController(std::shared_ptr<const services::inventory::ISystemInventoryService> service)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        throw std::invalid_argument("InventoryController: service must not be null");
    }
}

/**
 * @brief Register the three GET routes.
 * @param registry Target registry.
 */
void InventoryController::registerRoutes(IRouteRegistry& registry)
{
    registry.addRoute(HttpMethod::Get, kServicesRoute,
                      [this](const Request& request) { return handleGetServices(request); });
    registry.addRoute(HttpMethod::Get, kStartupRoute,
                      [this](const Request& request) { return handleGetStartup(request); });
    registry.addRoute(HttpMethod::Get, kUsersRoute,
                      [this](const Request& request) { return handleGetUsers(request); });
}

/**
 * @brief Handle GET kServicesRoute.
 * @param request Incoming request (unused).
 * @return 200 with a ServiceList object.
 */
Response InventoryController::handleGetServices(const Request& /*request*/) const
{
    const auto inventory = m_service->getServices();
    json body = availabilityJson(inventory.available, inventory.reason);
    json services = json::array();
    for (const auto& service : inventory.items)
    {
        services.push_back({
            {"name", service.name},
            {"displayName", service.displayName},
            {"description", nullable(service.description)},
            {"state", toString(service.state)},
            {"startType", toString(service.startType)},
            {"pid", nullable(service.pid)},
        });
    }
    body["services"] = std::move(services);
    return ok(body);
}

/**
 * @brief Handle GET kStartupRoute.
 * @param request Incoming request (unused).
 * @return 200 with a StartupList object.
 */
Response InventoryController::handleGetStartup(const Request& /*request*/) const
{
    const auto inventory = m_service->getStartupItems();
    json body = availabilityJson(inventory.available, inventory.reason);
    json items = json::array();
    for (const auto& item : inventory.items)
    {
        items.push_back({
            {"name", item.name},
            {"command", item.command},
            {"location", item.location},
            {"scope", item.scope == providers::StartupScope::User ? "user" : "allUsers"},
            {"enabled", item.enabled},
        });
    }
    body["items"] = std::move(items);
    return ok(body);
}

/**
 * @brief Handle GET kUsersRoute.
 * @param request Incoming request (unused).
 * @return 200 with a UserList object.
 * @throws std::runtime_error if no metrics are available (becomes a 500 problem).
 */
Response InventoryController::handleGetUsers(const Request& /*request*/) const
{
    const auto overview = m_service->getUsers();
    json body = availabilityJson(overview.sessions.available, overview.sessions.reason);
    json sessions = json::array();
    for (const auto& session : overview.sessions.items)
    {
        json logonTime = nullptr;
        if (session.logonTime)
        {
            logonTime = toRfc3339(*session.logonTime);
        }
        sessions.push_back({
            {"userName", session.userName},
            {"sessionId", nullable(session.sessionId)},
            {"sessionName", session.sessionName},
            {"state", session.state},
            {"logonTime", std::move(logonTime)},
            {"host", nullable(session.host)},
        });
    }
    json users = json::array();
    for (const auto& user : overview.users)
    {
        users.push_back({
            {"userName", user.userName},
            {"processCount", user.processCount},
            {"cpuPercent", user.cpuPercent},
            {"memoryBytes", user.memoryBytes},
        });
    }
    body["sessions"] = std::move(sessions);
    body["users"] = std::move(users);
    return ok(body);
}

} // namespace systemlens::api::inventory
