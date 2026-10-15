/**
 * @file    ApiResponse.cpp
 * @brief   Implementation of the response builders declared in ApiResponse.h.
 */
#include "api/ApiResponse.h"

namespace systemlens::api {

Response ok(const nlohmann::json& body, int statusCode)
{
    return Response{statusCode, "application/json", body.dump()};
}

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
