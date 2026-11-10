/**
 * @file    SystemController.cpp
 * @brief   Implementation of SystemController.
 */
#include "api/system/SystemController.h"

#include "api/ApiResponse.h"
#include "api/JsonHelpers.h"

#include <stdexcept>

namespace systemlens::api::system {

namespace {

/**
 * @brief Serialize the overview as the SystemOverview schema.
 * @param overview Service result.
 * @return JSON object.
 */
nlohmann::json toJson(const services::system::SystemOverview& overview)
{
    nlohmann::json topMemoryProcess = nullptr;
    if (overview.topMemoryProcess)
    {
        topMemoryProcess = {
            {"name", overview.topMemoryProcess->name},
            {"memoryBytes", overview.topMemoryProcess->memoryBytes},
        };
    }
    return {
        {"machine",
         {
             {"hostName", overview.machine.hostName},
             {"osName", overview.machine.osName},
             {"osVersion", overview.machine.osVersion},
             {"architecture", overview.machine.architecture},
             {"userName", overview.machine.userName},
             {"sessionName", overview.machine.sessionName},
         }},
        {"bootTime", toRfc3339(overview.bootTime)},
        {"uptimeSeconds", overview.uptime.count()},
        {"sampleIntervalMs", overview.sampleInterval.count()},
        {"processCount", overview.processCount},
        {"serviceCount", nullable(overview.serviceCount)},
        {"cpuPercent", overview.cpuPercent},
        {"memoryPercent", overview.memoryPercent},
        {"diskPercent", nullable(overview.diskPercent)},
        {"network",
         {
             {"sendBitsPerSecond", overview.sendBitsPerSecond},
             {"receiveBitsPerSecond", overview.receiveBitsPerSecond},
         }},
        {"topMemoryProcess", std::move(topMemoryProcess)},
    };
}

} // namespace

/**
 * @brief Create the controller.
 * @param service Provides the overview. Must not be null.
 * @throws std::invalid_argument if service is null.
 */
SystemController::SystemController(std::shared_ptr<const services::system::ISystemOverviewService> service)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        throw std::invalid_argument("SystemController: service must not be null");
    }
}

/**
 * @brief Register GET kOverviewRoute.
 * @param registry Target registry.
 */
void SystemController::registerRoutes(IRouteRegistry& registry)
{
    registry.addRoute(HttpMethod::Get, kOverviewRoute,
                      [this](const Request& request) { return handleGetOverview(request); });
}

/**
 * @brief Handle GET kOverviewRoute.
 * @param request Incoming request (unused: the endpoint takes no input).
 * @return 200 with a SystemOverview object.
 * @throws std::runtime_error if no metrics are available (becomes a 500 problem).
 */
Response SystemController::handleGetOverview(const Request& /*request*/) const
{
    return ok(toJson(m_service->getOverview()));
}

} // namespace systemlens::api::system
