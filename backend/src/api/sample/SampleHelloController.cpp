/**
 * @file    SampleHelloController.cpp
 * @brief   [SAMPLE API] Implementation of SampleHelloController.
 */
#include "api/sample/SampleHelloController.h"

#include "api/ApiResponse.h"

#include <stdexcept>

namespace systemlens::api::sample {

/**
 * @brief Create the controller.
 * @param service Provides the greeting. Must not be null.
 */
SampleHelloController::SampleHelloController(std::shared_ptr<const services::sample::ISampleHelloService> service)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        throw std::invalid_argument("SampleHelloController: service must not be null");
    }
}

/**
 * @brief Register GET kRoute.
 * @param registry Target registry.
 */
void SampleHelloController::registerRoutes(IRouteRegistry& registry)
{
    registry.addRoute(HttpMethod::Get, kRoute, [this](const Request& request) { return handleGetHello(request); });
}

/**
 * @brief Handle GET kRoute.
 * @param request Incoming request (unused: the endpoint takes no input).
 * @return 200 with {"message": <greeting>}.
 */
Response SampleHelloController::handleGetHello(const Request& /*request*/) const
{
    return ok({{"message", m_service->getGreeting()}});
}

} // namespace systemlens::api::sample
