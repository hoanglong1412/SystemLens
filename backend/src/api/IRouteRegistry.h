/**
 * @file    IRouteRegistry.h
 * @brief   Interface used by controllers to register their routes.
 *
 * HttpServer implements it; controllers depend only on this interface, not on the server.
 */
#pragma once

#include "api/HttpTypes.h"

#include <string>

namespace systemlens::api {

class IRouteRegistry
{
public:
    virtual ~IRouteRegistry() = default;

    /**
     * @brief Register a handler for an exact method + path pair.
     * @param method  HTTP method to match.
     * @param path    Exact request path, e.g. "/api/v1/sample/hello".
     * @param handler Called for every matching request.
     */
    virtual void addRoute(HttpMethod method, const std::string& path, Handler handler) = 0;
};

} // namespace systemlens::api
