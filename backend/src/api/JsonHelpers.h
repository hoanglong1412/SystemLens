/**
 * @file    JsonHelpers.h
 * @brief   Conversions shared by controllers when they turn service results into JSON.
 */
#pragma once

#include <nlohmann/json.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::api {

std::string toRfc3339(std::chrono::system_clock::time_point time);

/**
 * @brief Convert an optional value to JSON.
 * @param value Optional value of any JSON-convertible type.
 * @return The value, or JSON null when empty.
 */
template <typename T>
nlohmann::json nullable(const std::optional<T>& value)
{
    return value ? nlohmann::json(*value) : nlohmann::json(nullptr);
}

/**
 * @brief Convert a series with gaps to a JSON array.
 * @param values Samples; empty optionals mark gaps.
 * @return Array with JSON null at the gaps.
 */
template <typename T>
nlohmann::json nullableArray(const std::vector<std::optional<T>>& values)
{
    nlohmann::json array = nlohmann::json::array();
    for (const std::optional<T>& value : values)
    {
        array.push_back(nullable(value));
    }
    return array;
}

} // namespace systemlens::api
