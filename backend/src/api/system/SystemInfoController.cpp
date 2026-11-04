/**
 * @file    SystemInfoController.cpp
 * @brief   Implementation of SystemInfoController.
 */
#include "api/system/SystemInfoController.h"

#include "api/ApiResponse.h"
#include "services/FeatureUnavailableError.h"

#include <stdexcept>

namespace systemlens::api::system {

/**
 * @brief Create the controller.
 * @param service System info service. Must not be null.
 * @throws std::invalid_argument when service is null.
 */
SystemInfoController::SystemInfoController(std::shared_ptr<const services::system::ISystemInfoService> service)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        throw std::invalid_argument("SystemInfoController: service must not be null");
    }
}

/**
 * @brief Register GET kRoute.
 * @param registry Target registry.
 */
void SystemInfoController::registerRoutes(IRouteRegistry& registry)
{
    registry.addRoute(HttpMethod::Get, kRoute, [this](const Request& request) { return handleGetInfo(request); });
}

/**
 * @brief Handle GET kRoute.
 * @param request Incoming request (unused: no input).
 * @return 200 SystemInfo, or 501 when the platform has no system info provider.
 */
Response SystemInfoController::handleGetInfo(const Request& /*request*/) const
{
    try
    {
        const auto identity = m_service->getIdentity();
        return ok({
            {"hostName", identity.hostName},
            {"osName", identity.osName},
            {"osVersion", identity.osVersion},
            {"userName", identity.userName},
            {"platform", identity.platform},
        });
    }
    catch (const services::FeatureUnavailableError& e)
    {
        return problem(status::kNotImplemented, "Not Implemented", e.what());
    }
}

} // namespace systemlens::api::system
