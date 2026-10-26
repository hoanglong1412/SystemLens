/**
 * @file    SampleHelloController.h
 * @brief   [SAMPLE API] Controller for GET /api/v1/sample/hello (contract: api/openapi.yaml).
 *
 * Reference example for new controllers: no business logic, only
 * Request -> service call -> JSON Response.
 */
#pragma once

#include "api/IController.h"
#include "services/sample/ISampleHelloService.h"

#include <memory>

namespace systemlens::api::sample {

class SampleHelloController final : public IController
{
public:
    /** Path served by this controller. */
    static constexpr const char* kRoute = "/api/v1/sample/hello";

    explicit SampleHelloController(std::shared_ptr<const services::sample::ISampleHelloService> service);
    void registerRoutes(IRouteRegistry& registry) override;

private:
    Response handleGetHello(const Request& request) const;

    std::shared_ptr<const services::sample::ISampleHelloService> m_service;
};

} // namespace systemlens::api::sample
