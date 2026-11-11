/**
 * @file    ICpuProvider.h
 * @brief   Provider interface for processor facts and raw CPU counters.
 *
 * Counters are cumulative since boot; the caller turns two samples into rates.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::providers {

/** Whether hardware virtualization is usable. */
enum class VirtualizationState
{
    Enabled,
    Disabled,
    Unknown,
};

/** Facts about the processor that do not change while the backend runs. */
struct CpuStaticInfo
{
    std::string model;
    std::uint32_t sockets = 0;
    std::uint32_t cores = 0;
    std::uint32_t logicalProcessors = 0;
    std::optional<double> baseSpeedMhz;
    std::optional<double> maxSpeedMhz; ///< Highest clock the OS reports (turbo), when known.
    VirtualizationState virtualization = VirtualizationState::Unknown;
    std::optional<std::uint64_t> l1CacheBytes; ///< Sum of all L1 data + instruction caches.
    std::optional<std::uint64_t> l2CacheBytes;
    std::optional<std::uint64_t> l3CacheBytes;
    std::string activitySource; ///< Where CpuActivityCounters come from, shown in the UI.
};

/** Cumulative CPU time of all logical processors together. The five parts do not overlap. */
struct CpuTimes
{
    using Duration = std::chrono::duration<double>;

    Duration user{};      ///< Application code.
    Duration kernel{};    ///< Kernel code, excluding dpc and interrupt.
    Duration dpc{};       ///< Windows: DPC; Linux: softirq.
    Duration interrupt{}; ///< Hardware interrupt handlers.
    Duration idle{};      ///< Nothing to run (Linux: includes iowait and steal).
};

/** Cumulative system-wide event counts; nullopt when the platform has no such counter. */
struct CpuActivityCounters
{
    std::optional<std::uint64_t> contextSwitches;
    std::optional<std::uint64_t> interrupts;
    std::optional<std::uint64_t> systemCalls;
    std::optional<std::uint64_t> dpcs; ///< Windows: DPCs queued; Linux: softirqs raised.
};

/** Everything that is read from the CPU on every sampling tick. */
struct CpuSample
{
    CpuTimes times;
    CpuActivityCounters activity;
    std::vector<CpuTimes> perCore; ///< One entry per logical processor, in index order.
    std::optional<double> frequencyMhz;       ///< Current clock averaged over logical processors.
    std::optional<double> temperatureCelsius; ///< Package temperature.
};

class ICpuProvider
{
public:
    virtual ~ICpuProvider() = default;

    /**
     * @brief Read the processor facts (model, topology, caches).
     * @return Static information; call once and cache.
     * @throws ProviderError on failure.
     */
    [[nodiscard]] virtual CpuStaticInfo readStaticInfo() const = 0;

    /**
     * @brief Read the counters and sensors. Meant to be called once per sampling interval:
     *        implementations may measure "current" values between two consecutive calls.
     * @return Current sample.
     * @throws ProviderError if the CPU times cannot be read.
     */
    [[nodiscard]] virtual CpuSample readSample() = 0;
};

} // namespace systemlens::providers
