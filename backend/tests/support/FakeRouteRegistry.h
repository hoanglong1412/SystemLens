/**
 * @file    FakeRouteRegistry.h
 * @brief   Test double for IRouteRegistry: records registered routes so tests can call
 *          handlers directly, without any HTTP server.
 */
#pragma once

#include "api/IRouteRegistry.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace systemlens::tests {

class FakeRouteRegistry final : public api::IRouteRegistry
{
public:
    /** One registered route. */
    struct Route
    {
        api::HttpMethod method;
        std::string path;
        api::Handler handler;
    };

    /** @brief Record the route (see IRouteRegistry::addRoute). */
    void addRoute(api::HttpMethod method, const std::string& path, api::Handler handler) override
    {
        m_routes.push_back(Route{method, path, std::move(handler)});
    }

    /** @brief All routes registered so far, in registration order. */
    const std::vector<Route>& routes() const { return m_routes; }

    /**
     * @brief Invoke the handler registered for method + path.
     * @param method  HTTP method.
     * @param path    Exact path.
     * @param request Request passed to the handler.
     * @return The handler's response.
     * @throws std::out_of_range if no such route was registered.
     */
    api::Response call(api::HttpMethod method, const std::string& path, const api::Request& request = {}) const
    {
        for (const auto& route : m_routes)
        {
            if (route.method == method && route.path == path)
            {
                return route.handler(request);
            }
        }
        throw std::out_of_range("No route registered for " + path);
    }

private:
    std::vector<Route> m_routes;
};

} // namespace systemlens::tests
