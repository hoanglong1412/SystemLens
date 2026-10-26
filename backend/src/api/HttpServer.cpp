/**
 * @file    HttpServer.cpp
 * @brief   Implementation of HttpServer on top of cpp-httplib.
 */
#include "api/HttpServer.h"

#include "api/ApiResponse.h"

#include <httplib.h>

#include <iostream>
#include <stdexcept>

namespace systemlens::api {

namespace {

constexpr const char* kAuthHeader = "Authorization";
constexpr const char* kBearerPrefix = "Bearer ";

/**
 * @brief Compare two strings in time that does not depend on where they differ.
 *
 * A plain == stops at the first different character, which in theory lets an
 * attacker guess the token one character at a time by measuring response time.
 */
bool constantTimeEquals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); ++i)
    {
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return diff == 0;
}

/**
 * @brief Add the CORS headers the Electron renderer needs.
 *
 * The renderer is loaded from file://, so its origin is "null" and every fetch to
 * http://127.0.0.1 is cross-origin. "*" is acceptable here because the API only
 * listens on loopback and every call still needs the bearer token.
 */
void addCorsHeaders(httplib::Response& res)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Headers", "Authorization, Content-Type");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
}

/** @brief Copy a project Response into the cpp-httplib response object. */
void writeResponse(const Response& from, httplib::Response& to)
{
    to.status = from.status;
    to.set_content(from.body, from.contentType);
}

/** @brief Convert a cpp-httplib request into the library-independent Request type. */
Request toRequest(const httplib::Request& req)
{
    return Request{req.path, req.body};
}

} // namespace

HttpServer::HttpServer(HttpServerOptions options)
    : m_options(std::move(options))
    , m_server(std::make_unique<httplib::Server>())
{
    installMiddleware();
}

HttpServer::~HttpServer()
{
    stop();
}

void HttpServer::addRoute(HttpMethod method, const std::string& path, Handler handler)
{
    // Wrap the controller handler so it works with cpp-httplib types. Exceptions are
    // not caught here: the exception handler installed in installMiddleware() does it once for all routes.
    auto adapter = [handler = std::move(handler)](const httplib::Request& req, httplib::Response& res) {
        writeResponse(handler(toRequest(req)), res);
    };

    switch (method)
    {
    case HttpMethod::Get:
        m_server->Get(path, adapter);
        break;
    case HttpMethod::Post:
        m_server->Post(path, adapter);
        break;
    case HttpMethod::Put:
        m_server->Put(path, adapter);
        break;
    case HttpMethod::Delete:
        m_server->Delete(path, adapter);
        break;
    }
}

int HttpServer::bind()
{
    if (m_options.port == 0)
    {
        const int port = m_server->bind_to_any_port(m_options.host);
        if (port < 0)
        {
            throw std::runtime_error("Cannot bind " + m_options.host + " to any port");
        }
        return port;
    }

    if (!m_server->bind_to_port(m_options.host, m_options.port))
    {
        throw std::runtime_error("Cannot bind " + m_options.host + ":" + std::to_string(m_options.port));
    }
    return m_options.port;
}

void HttpServer::listen()
{
    m_server->listen_after_bind();
}

void HttpServer::stop()
{
    if (m_server && m_server->is_running())
    {
        m_server->stop();
    }
}

void HttpServer::installMiddleware()
{
    // Runs before routing: answers CORS preflight and rejects unauthenticated requests,
    // so individual controllers never have to think about auth.
    m_server->set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);

        // Browsers send the preflight without the Authorization header, so it must be answered before the auth check.
        if (req.method == "OPTIONS")
        {
            res.status = status::kNoContent;
            return httplib::Server::HandlerResponse::Handled;
        }

        if (m_options.token)
        {
            const std::string expected = std::string(kBearerPrefix) + *m_options.token;
            if (!constantTimeEquals(req.get_header_value(kAuthHeader), expected))
            {
                writeResponse(problem(status::kUnauthorized, "Unauthorized", "Missing or invalid bearer token."), res);
                return httplib::Server::HandlerResponse::Handled;
            }
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Called for error statuses. Only fills in a body when the route did not set one (e.g. unknown route → 404).
    m_server->set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        if (res.body.empty() && res.status == status::kNotFound)
        {
            writeResponse(problem(status::kNotFound, "Not Found", "No route for " + req.method + " " + req.path), res);
        }
    });

    // Any exception escaping a handler becomes a 500 problem instead of crashing the server.
    m_server->set_exception_handler([](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep) {
        std::string detail = "Unknown error";
        try
        {
            std::rethrow_exception(ep);
        }
        catch (const std::exception& e)
        {
            detail = e.what();
        }
        catch (...)
        {
        }
        // stdout is reserved for the READY handshake, so logs go to stderr.
        std::cerr << "[HttpServer] " << req.method << " " << req.path << " failed: " << detail << std::endl;
        addCorsHeaders(res);
        writeResponse(problem(status::kInternalServerError, "Internal Server Error", detail), res);
    });
}

} // namespace systemlens::api
