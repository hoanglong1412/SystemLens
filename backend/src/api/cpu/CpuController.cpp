/**
 * @file    CpuController.cpp
 * @brief   Implementation of CpuController.
 */
#include "api/cpu/CpuController.h"

#include "api/ApiResponse.h"
#include "api/cpu/CpuJson.h"
#include "services/FeatureUnavailableError.h"
#include "services/NotReadyError.h"

#include <stdexcept>

namespace systemlens::api::cpu {

namespace {

/**
 * @brief Run a handler body and map the service errors of the CPU endpoints to problems.
 * @param body Builds the success response.
 * @return body() result, 501 when the platform has no CPU provider, 503 before the first sample.
 */
template <typename Body>
Response withServiceErrors(Body&& body)
{
    try
    {
        return body();
    }
    catch (const services::FeatureUnavailableError& e)
    {
        return problem(status::kNotImplemented, "Not Implemented", e.what());
    }
    catch (const services::NotReadyError& e)
    {
        // 503: the request is valid and will succeed shortly; the client simply retries on its next poll.
        return problem(status::kServiceUnavailable, "Service Unavailable", e.what());
    }
}

} // namespace

/**
 * @brief Create the controller.
 * @param service CPU monitor. Must not be null.
 * @throws std::invalid_argument when service is null.
 */
CpuController::CpuController(std::shared_ptr<const services::cpu::ICpuMonitorService> service)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        throw std::invalid_argument("CpuController: service must not be null");
    }
}

/**
 * @brief Register GET kInfoRoute and GET kMetricsRoute.
 * @param registry Target registry.
 */
void CpuController::registerRoutes(IRouteRegistry& registry)
{
    registry.addRoute(HttpMethod::Get, kInfoRoute, [this](const Request& request) { return handleGetInfo(request); });
    registry.addRoute(HttpMethod::Get, kMetricsRoute,
                      [this](const Request& request) { return handleGetMetrics(request); });
}

/**
 * @brief Handle GET kInfoRoute.
 * @param request Incoming request (unused: no input).
 * @return 200 CpuInfo, 501 or 503 problem.
 */
Response CpuController::handleGetInfo(const Request& /*request*/) const
{
    return withServiceErrors([this] { return ok(toJson(m_service->getInfo())); });
}

/**
 * @brief Handle GET kMetricsRoute.
 * @param request Incoming request (unused: no input).
 * @return 200 CpuMetrics, 501 or 503 problem.
 */
Response CpuController::handleGetMetrics(const Request& /*request*/) const
{
    return withServiceErrors([this] { return ok(toJson(m_service->getMetrics())); });
}

} // namespace systemlens::api::cpu
