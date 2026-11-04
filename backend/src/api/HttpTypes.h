/**
 * @file    HttpTypes.h
 * @brief   Library-independent HTTP request/response types used by controllers.
 *
 * Controllers only see these types, never cpp-httplib. Swapping the HTTP library
 * later therefore only touches HttpServer.cpp.
 */
#pragma once

#include <functional>
#include <string>

namespace systemlens::api {

/** HTTP methods supported by the route registry. */
enum class HttpMethod
{
    Get,
    Post,
    Put,
    Delete,
};

/** Incoming request, reduced to what controllers need. */
struct Request
{
    std::string path;  ///< Request path without query string, e.g. "/api/v1/sample/hello".
    std::string body;  ///< Raw request body (JSON for POST/PUT).
};

/** Outgoing response. Build it with the helpers in ApiResponse.h. */
struct Response
{
    int status = 200;                             ///< HTTP status code.
    std::string contentType = "application/json"; ///< Value of the Content-Type header.
    std::string body;                             ///< Serialized body.
};

/** Function that turns a request into a response. May throw; HttpServer converts exceptions to 500. */
using Handler = std::function<Response(const Request&)>;

} // namespace systemlens::api
