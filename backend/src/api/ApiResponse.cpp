/**
 * @file    ApiResponse.cpp
 * @brief   Implementation of the response builders declared in ApiResponse.h.
 */
#include "api/ApiResponse.h"

namespace systemlens::api {

/**
 * @brief Build a success response with a JSON body.
 * @param body   Resource to serialize.
 * @param statusCode HTTP status code (default 200).
 * @return Response with Content-Type application/json.
 */
Response ok(const nlohmann::json& body, int statusCode)
{
    return Response{statusCode, "application/json", body.dump()};
}

/**
 * @brief Build an RFC 7807 error response.
 * @param statusCode HTTP status code (4xx/5xx).
 * @param title  Short summary, e.g. "Unauthorized".
 * @param detail Explanation specific to this occurrence.
 * @return Response with Content-Type application/problem+json.
 */
Response problem(int statusCode, std::string_view title, std::string_view detail)
{
    // "about:blank" is the RFC 7807 default type: the title then simply describes the HTTP status.
    const nlohmann::json body = {
        {"type", "about:blank"},
        {"title", title},
        {"status", statusCode},
        {"detail", detail},
    };
    return Response{statusCode, "application/problem+json", body.dump()};
}

} // namespace systemlens::api
