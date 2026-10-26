/**
 * @file    HttpServer.h
 * @brief   HTTP server for the SystemLens API (wraps cpp-httplib).
 *
 * Responsibilities, applied to every request before/after controllers run:
 *  - bind to localhost only,
 *  - bearer-token authentication (skipped in dev mode),
 *  - CORS headers so the Electron renderer (origin "null", file://) can call the API,
 *  - converting exceptions and unknown routes into RFC 7807 problem responses.
 *
 * cpp-httplib types are only forward-declared here so that controllers
 * do not depend on the HTTP library.
 */
#pragma once

#include "api/IRouteRegistry.h"

#include <memory>
#include <optional>
#include <string>

namespace httplib {
class Server;
}

namespace systemlens::api {

/** Startup options for HttpServer. */
struct HttpServerOptions
{
    std::string host = "127.0.0.1";   ///< Interface to bind. Must stay a loopback address.
    int port = 0;                     ///< 0 = let the OS pick a free port.
    std::optional<std::string> token; ///< Required bearer token; std::nullopt disables auth (dev only).
};

class HttpServer final : public IRouteRegistry
{
public:
    explicit HttpServer(HttpServerOptions options);
    ~HttpServer() override;

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    void addRoute(HttpMethod method, const std::string& path, Handler handler) override;
    int bind();
    void listen();
    void stop();

private:
    void installMiddleware();

    HttpServerOptions m_options;
    std::unique_ptr<httplib::Server> m_server;
};

} // namespace systemlens::api
