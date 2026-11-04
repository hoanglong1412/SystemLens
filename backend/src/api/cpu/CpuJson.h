/**
 * @file    CpuJson.h
 * @brief   JSON serialization of the CPU resources (schemas CpuInfo and CpuMetrics in api/openapi.yaml).
 *          Missing values are written as null, never omitted, so every response has the same shape.
 */
#pragma once

#include "providers/cpu/ICpuProvider.h"
#include "services/cpu/CpuMetrics.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace systemlens::api::cpu {

nlohmann::json toJson(const providers::cpu::CpuStaticInfo& info);

nlohmann::json toJson(const services::cpu::CpuMetrics& metrics);

std::string toIso8601Utc(std::int64_t unixSeconds);

} // namespace systemlens::api::cpu
