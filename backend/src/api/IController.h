/**
 * @file    IController.h
 * @brief   Interface implemented by every API controller.
 *
 * A controller translates HTTP (Request/Response) to service calls and back.
 * It owns no business logic: that lives in the services layer.
 */
#pragma once

#include "api/IRouteRegistry.h"

namespace systemlens::api {

class IController
{
public:
    virtual ~IController() = default;

    /**
     * @brief Register all routes handled by this controller.
     * @param registry Target registry (normally the HttpServer).
     */
    virtual void registerRoutes(IRouteRegistry& registry) = 0;
};

} // namespace systemlens::api
