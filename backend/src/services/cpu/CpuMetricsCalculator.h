/**
 * @file    CpuMetricsCalculator.h
 * @brief   Pure arithmetic that turns two reads of cumulative CPU counters into percentages and
 *          per-second rates (design D3). No OS access, no state: unit tested with plain structs.
 */
#pragma once

#include "providers/cpu/ICpuProvider.h"
#include "services/cpu/CpuMetrics.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace systemlens::services::cpu {

/** Number of rows of the top CPU processes table. */
inline constexpr std::size_t kTopProcessCount = 10;

std::optional<std::uint64_t> counterDelta(std::uint64_t previous, std::uint64_t current);

CpuBreakdown computeBreakdown(const providers::cpu::CpuTimes& previous, const providers::cpu::CpuTimes& current);

CpuSnapshot computeSnapshot(const providers::cpu::CpuCounters& previous, const providers::cpu::CpuCounters& current,
                            double elapsedSeconds, std::uint32_t logicalProcessors);

} // namespace systemlens::services::cpu
