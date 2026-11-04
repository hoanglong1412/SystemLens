/**
 * @file    FakeRouteRegistry.h
 * @brief   [TEST] IRouteRegistry that stores the registered handlers so tests can call a route
 *          without starting the HTTP server.
 */
#pragma once

#include "api/IRouteRegistry.h"

#include <map>
#include <stdexcept>
#include <string>
#include <utility>

namespace systemlens::tests {

class FakeRouteRegistry final : public api::IRouteRegistry
{
public:
    /**
     * @brief Store a handler.
     * @param method HTTP method.
     * @param path Exact path.
     * @param handler Handler to store.
     */
    void addRoute(api::HttpMethod method, const std::string& path, api::Handler handler) override
    {
        m_routes[{method, path}] = std::move(handler);
    }

    /**
     * @brief Whether a route was registered.
     * @param method HTTP method.
     * @param path Exact path.
     * @return true when registered.
     */
    bool has(api::HttpMethod method, const std::string& path) const
    {
        return m_routes.count({method, path}) == 1;
    }

    /**
     * @brief Call a registered GET handler.
     * @param path Exact path.
     * @return Handler response.
     * @throws std::out_of_range when the route is not registered.
     */
    api::Response get(const std::string& path) const
    {
        return m_routes.at({api::HttpMethod::Get, path})(api::Request{path, ""});
    }

private:
    std::map<std::pair<api::HttpMethod, std::string>, api::Handler> m_routes;
};

} // namespace systemlens::tests
