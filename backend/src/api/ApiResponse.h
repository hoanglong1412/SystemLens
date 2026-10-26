/**
 * @file    ApiResponse.h
 * @brief   The only place where API responses are built, so every endpoint has the same shape.
 *
 * Success: JSON body of the resource (application/json).
 * Error  : RFC 7807 problem object (application/problem+json). See docs/api-convention.md.
 */
#pragma once

#include "api/HttpTypes.h"

#include <nlohmann/json.hpp>

#include <string_view>

namespace systemlens::api {

/** HTTP status codes used by the API (avoids magic numbers in controllers). */
namespace status {
inline constexpr int kOk = 200;
inline constexpr int kNoContent = 204;
inline constexpr int kBadRequest = 400;
inline constexpr int kUnauthorized = 401;
inline constexpr int kNotFound = 404;
inline constexpr int kInternalServerError = 500;
} // namespace status

Response ok(const nlohmann::json& body, int statusCode = status::kOk);

Response problem(int statusCode, std::string_view title, std::string_view detail);

} // namespace systemlens::api
