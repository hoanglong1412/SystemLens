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
 * cpp-httplib types are only forward-declared here so that controllers and tests
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
    /**
     * @brief Create the server and install auth/CORS/error middleware. Does not bind yet.
     * @param options Host, port and token.
     */
    explicit HttpServer(HttpServerOptions options);

    /** @brief Stop the server if still running. */
    ~HttpServer() override;

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    /**
     * @brief Register a handler (see IRouteRegistry). Must be called before listen().
     * @param method  HTTP method.
     * @param path    Exact path.
     * @param handler Request handler; exceptions it throws become 500 responses.
     */
    void addRoute(HttpMethod method, const std::string& path, Handler handler) override;

    /**
     * @brief Bind the listening socket.
     * @return The bound port (the real one when options.port was 0).
     * @throws std::runtime_error if the address/port cannot be bound.
     */
    int bind();

    /**
     * @brief Serve requests until stop() is called. Blocks the calling thread.
     * @pre bind() succeeded.
     */
    void listen();

    /** @brief Block until listen() is accepting connections (used by tests that run listen() on a thread). */
    void waitUntilReady() const;

    /** @brief Ask listen() to return. Safe to call from any thread, and more than once. */
    void stop();

private:
    /** @brief Install the auth/CORS pre-routing handler and the error/exception handlers. */
    void installMiddleware();

    HttpServerOptions m_options;
    std::unique_ptr<httplib::Server> m_server;
};

} // namespace systemlens::api
