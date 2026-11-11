/**
 * @file    MetricsCollector.h
 * @brief   Samples the platform providers on a background thread and publishes MetricsSnapshots.
 *
 * Providers return raw cumulative counters; this class turns consecutive samples into
 * percentages and per-second rates and keeps the 60-second histories, so every platform
 * shares the same arithmetic.
 */
#pragma once

#include "providers/PlatformProviders.h"
#include "services/monitoring/History.h"
#include "services/monitoring/IMetricsSource.h"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace systemlens::services::monitoring {

/** Time between two sampling ticks. */
inline constexpr std::chrono::milliseconds kDefaultSampleInterval{1000};
/** Samples kept per history: 60 intervals need 61 points. */
inline constexpr std::size_t kHistoryCapacity = 61;
/** Rows of the "Top CPU processes" table. */
inline constexpr std::size_t kTopProcessCount = 10;
/** Applications kept in the App history list. */
inline constexpr std::size_t kAppHistoryLimit = 200;

class MetricsCollector final : public IMetricsSource
{
public:
    explicit MetricsCollector(providers::PlatformProviders providers,
                              std::chrono::milliseconds sampleInterval = kDefaultSampleInterval);
    ~MetricsCollector() override;

    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;

    void start();
    void stop();
    [[nodiscard]] std::shared_ptr<const MetricsSnapshot> latest() const override;

private:
    using SteadyTime = std::chrono::steady_clock::time_point;
    using Seconds = std::chrono::duration<double>;

    /** Values that never change while the backend runs; read on the first successful tick. */
    struct StaticData
    {
        providers::SystemIdentity identity;
        std::chrono::system_clock::time_point bootTime;
        providers::CpuStaticInfo cpuInfo;
    };

    /** Cumulative bytes of one disk. */
    struct DiskIo
    {
        std::uint64_t bytesRead = 0;
        std::uint64_t bytesWritten = 0;
    };

    /** Usage of one application name since the collector started. */
    struct AppAccumulator
    {
        double cpuTimeSeconds = 0.0;
        std::uint64_t peakMemoryBytes = 0;
        std::uint64_t memoryBytes = 0;
        std::uint32_t runningInstances = 0;
    };

    /** Raw counters of the previous tick, needed to compute deltas. */
    struct PreviousSample
    {
        SteadyTime takenAt;
        providers::CpuSample cpu;
        std::unordered_map<std::string, Seconds> diskBusyTimes;
        std::unordered_map<std::string, DiskIo> diskIo;
        std::unordered_map<std::string, providers::NetworkSample> networks;
        std::unordered_map<std::uint32_t, Seconds> processCpuTimes;
    };

    /** Histories of the CPU series. */
    struct CpuHistories
    {
        History<double> usage{kHistoryCapacity};
        History<double> user{kHistoryCapacity};
        History<double> kernel{kHistoryCapacity};
        History<std::optional<double>> frequencyPercent{kHistoryCapacity};
        History<std::optional<double>> temperature{kHistoryCapacity};
    };

    void run(std::stop_token stopToken);
    void tick();
    [[nodiscard]] MetricsSnapshot sample();
    void sampleCpu(const providers::CpuSample& current, Seconds elapsed, MetricsSnapshot& snapshot);
    void sampleMemory(MetricsSnapshot& snapshot);
    void sampleDisks(Seconds elapsed, MetricsSnapshot& snapshot);
    void sampleNetworks(Seconds elapsed, MetricsSnapshot& snapshot);
    void sampleGpus(MetricsSnapshot& snapshot);
    void sampleProcesses(Seconds elapsed, MetricsSnapshot& snapshot);
    [[nodiscard]] History<double>& historyFor(const std::string& id);

    providers::PlatformProviders m_providers;
    std::chrono::milliseconds m_sampleInterval;

    // ---- state owned by the sampling thread (no locking needed) ----
    std::optional<StaticData> m_static;
    std::optional<PreviousSample> m_previous;
    PreviousSample m_current; ///< Counters of the tick in progress; becomes m_previous when it succeeds.
    CpuHistories m_cpuHistories;
    History<double> m_memoryHistory{kHistoryCapacity};
    std::vector<History<double>> m_coreHistories; ///< One per logical processor.
    std::unordered_map<std::string, AppAccumulator> m_apps; ///< By process name.
    std::chrono::system_clock::time_point m_appsSince = std::chrono::system_clock::now();
    std::map<std::string, History<double>> m_deviceHistories; ///< Disks, adapters and GPUs by id.
    double m_highestFrequencyMhz = 0.0;                       ///< Scale of the clock gauge.
    bool m_failureLogged = false;

    // ---- state shared with request threads ----
    mutable std::mutex m_mutex;
    std::shared_ptr<const MetricsSnapshot> m_latest;
    std::string m_lastError; ///< Empty when the most recent tick succeeded.

    std::condition_variable_any m_wakeUp;
    std::mutex m_wakeUpMutex;
    std::jthread m_thread; // Last member: stops and joins before the state above is destroyed.
};

} // namespace systemlens::services::monitoring
