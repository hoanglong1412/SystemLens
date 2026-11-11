/**
 * @file    ProcessController.cpp
 * @brief   Implementation of ProcessController.
 */
#include "api/processes/ProcessController.h"

#include "api/ApiResponse.h"
#include "api/JsonHelpers.h"

#include <stdexcept>

namespace systemlens::api::processes {

namespace {

using nlohmann::json;

/**
 * @param text Possibly empty text.
 * @return The text, or JSON null when empty (the platform does not provide it).
 */
json nullIfEmpty(const std::string& text)
{
    return text.empty() ? json(nullptr) : json(text);
}

} // namespace

/**
 * @brief Create the controller.
 * @param service Provides the data. Must not be null.
 * @throws std::invalid_argument if service is null.
 */
ProcessController::ProcessController(std::shared_ptr<const services::processes::IProcessListService> service)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        throw std::invalid_argument("ProcessController: service must not be null");
    }
}

/**
 * @brief Register GET kProcessesRoute and GET kAppHistoryRoute.
 * @param registry Target registry.
 */
void ProcessController::registerRoutes(IRouteRegistry& registry)
{
    registry.addRoute(HttpMethod::Get, kProcessesRoute,
                      [this](const Request& request) { return handleGetProcesses(request); });
    registry.addRoute(HttpMethod::Get, kAppHistoryRoute,
                      [this](const Request& request) { return handleGetAppHistory(request); });
}

/**
 * @brief Handle GET kProcessesRoute.
 * @param request Incoming request (unused: the endpoint takes no input).
 * @return 200 with a ProcessList object.
 * @throws std::runtime_error if no metrics are available (becomes a 500 problem).
 */
Response ProcessController::handleGetProcesses(const Request& /*request*/) const
{
    json processes = json::array();
    for (const auto& process : m_service->getProcesses())
    {
        processes.push_back({
            {"pid", process.pid},
            {"parentPid", process.parentPid},
            {"name", process.name},
            {"userName", nullIfEmpty(process.userName)},
            {"cpuPercent", process.cpuPercent},
            {"cpuTimeSeconds", process.cpuTimeSeconds},
            {"memoryBytes", process.memoryBytes},
            // Zero threads means the platform did not report the count.
            {"threadCount", process.threadCount == 0 ? json(nullptr) : json(process.threadCount)},
            {"status", nullIfEmpty(process.status)},
            {"commandLine", nullIfEmpty(process.commandLine)},
            {"isSelf", process.isSelf},
        });
    }
    return ok({{"processes", std::move(processes)}});
}

/**
 * @brief Handle GET kAppHistoryRoute.
 * @param request Incoming request (unused: the endpoint takes no input).
 * @return 200 with an AppHistory object.
 * @throws std::runtime_error if no metrics are available (becomes a 500 problem).
 */
Response ProcessController::handleGetAppHistory(const Request& /*request*/) const
{
    const auto history = m_service->getAppHistory();
    json apps = json::array();
    for (const auto& app : history.apps)
    {
        apps.push_back({
            {"name", app.name},
            {"cpuTimeSeconds", app.cpuTimeSeconds},
            {"peakMemoryBytes", app.peakMemoryBytes},
            {"memoryBytes", app.memoryBytes},
            {"runningInstances", app.runningInstances},
        });
    }
    return ok({{"since", toRfc3339(history.since)}, {"apps", std::move(apps)}});
}

} // namespace systemlens::api::processes
