/**
 * @file    CpuMonitorService.h
 * @brief   ICpuMonitorService that samples the CPU provider on a background thread at a fixed
 *          interval and keeps the last N intervals (design D4).
 *
 * Threads: the sampling thread is the only caller of the provider; HTTP handler threads only read
 * the latest metrics, copied under m_mutex.
 */
#pragma once

#include "services/cpu/ICpuMonitorService.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace systemlens::services::cpu {

/** Sampling settings. */
struct CpuMonitorOptions
{
    std::chrono::milliseconds interval{1000}; ///< Time between two reads of the provider.
    std::uint32_t historyLength = 60;         ///< Number of intervals kept (60 s at 1 s).
};

class CpuMonitorService final : public ICpuMonitorService
{
public:
    CpuMonitorService(std::shared_ptr<providers::cpu::ICpuProvider> provider, CpuMonitorOptions options = {});
    ~CpuMonitorService() override;

    CpuMonitorService(const CpuMonitorService&) = delete;
    CpuMonitorService& operator=(const CpuMonitorService&) = delete;

    void start();
    void stop();

    providers::cpu::CpuStaticInfo getInfo() const override;
    CpuMetrics getMetrics() const override;

private:
    void runSampling();
    void sampleOnce();
    void appendHistory(const CpuSnapshot& snapshot);
    void requireProvider() const;
    void logProviderError(const std::string& message);

    std::shared_ptr<providers::cpu::ICpuProvider> m_provider;
    CpuMonitorOptions m_options;

    // Sampling-thread state (touched only by start() before the thread exists, then by the thread).
    std::optional<providers::cpu::CpuCounters> m_previous;
    std::chrono::steady_clock::time_point m_previousTime;
    std::string m_lastError;

    mutable std::mutex m_mutex; ///< Guards everything below.
    std::condition_variable m_wake;
    bool m_stopRequested = false;
    std::optional<providers::cpu::CpuStaticInfo> m_info;
    std::optional<CpuSnapshot> m_current;
    std::deque<CpuSnapshot> m_history;

    std::thread m_thread;
};

} // namespace systemlens::services::cpu
