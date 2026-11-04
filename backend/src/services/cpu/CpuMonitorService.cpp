/**
 * @file    CpuMonitorService.cpp
 * @brief   Implementation of CpuMonitorService.
 */
#include "services/cpu/CpuMonitorService.h"

#include "services/FeatureUnavailableError.h"
#include "services/NotReadyError.h"
#include "services/cpu/CpuMetricsCalculator.h"

#include <exception>
#include <iostream>
#include <stdexcept>

namespace systemlens::services::cpu {

namespace {

constexpr const char* kNoProviderMessage = "CPU data is not available on this platform";

} // namespace

/**
 * @brief Create the service. Does not read the provider or start the thread (see start()).
 * @param provider CPU provider of the platform; empty when the platform has none (endpoints answer 501).
 * @param options Sampling interval and history length.
 * @throws std::invalid_argument when the interval is not positive or the history length is 0.
 */
CpuMonitorService::CpuMonitorService(std::shared_ptr<providers::cpu::ICpuProvider> provider, CpuMonitorOptions options)
    : m_provider(std::move(provider))
    , m_options(options)
{
    if (m_options.interval.count() <= 0 || m_options.historyLength == 0)
    {
        throw std::invalid_argument("CpuMonitorService: interval and history length must be positive");
    }
}

/** @brief Stop the sampling thread if running. */
CpuMonitorService::~CpuMonitorService()
{
    stop();
}

/**
 * @brief Read the static info and the baseline counters, then start the sampling thread.
 *        Does nothing without a provider or when already started. Must be called before the
 *        service is shared with other threads (it may drop the provider).
 */
void CpuMonitorService::start()
{
    if (!m_provider || m_thread.joinable())
    {
        return;
    }

    providers::cpu::CpuStaticInfo info;
    try
    {
        info = m_provider->getStaticInfo();
    }
    catch (const std::exception& e)
    {
        // Same outcome as a missing provider (501) instead of stopping the whole backend.
        std::cerr << "[CpuMonitorService] WARNING: CPU provider disabled, static info failed: " << e.what()
                  << std::endl;
        m_provider.reset();
        return;
    }
    {
        std::lock_guard lock(m_mutex);
        m_info = std::move(info);
        m_stopRequested = false;
    }

    // A failing baseline is not fatal: the first tick takes a new baseline.
    try
    {
        m_previous = m_provider->readCounters();
        m_previousTime = std::chrono::steady_clock::now();
    }
    catch (const std::exception& e)
    {
        logProviderError(e.what());
    }

    m_thread = std::thread([this] { runSampling(); });
}

/** @brief Ask the sampling thread to finish and wait for it. Safe to call more than once. */
void CpuMonitorService::stop()
{
    {
        std::lock_guard lock(m_mutex);
        m_stopRequested = true;
    }
    m_wake.notify_all();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

/**
 * @brief Get the static processor facts.
 * @return Copy of the info read by start().
 * @throws FeatureUnavailableError without provider; NotReadyError before start().
 */
providers::cpu::CpuStaticInfo CpuMonitorService::getInfo() const
{
    requireProvider();
    std::lock_guard lock(m_mutex);
    if (!m_info)
    {
        throw NotReadyError("CPU monitor is not started");
    }
    return *m_info;
}

/**
 * @brief Get the latest metrics and the history.
 * @return Copy of the last snapshot and of the history arrays (oldest first).
 * @throws FeatureUnavailableError without provider; NotReadyError before the first interval.
 */
CpuMetrics CpuMonitorService::getMetrics() const
{
    requireProvider();
    std::lock_guard lock(m_mutex);
    if (!m_current)
    {
        throw NotReadyError("CPU metrics are not measured yet, retry in a second");
    }

    CpuMetrics metrics;
    metrics.sampleIntervalMs = static_cast<std::uint32_t>(m_options.interval.count());
    metrics.historyLength = m_options.historyLength;
    metrics.current = *m_current;
    for (const auto& snapshot : m_history)
    {
        metrics.history.usagePercent.push_back(snapshot.usagePercent);
        metrics.history.speedMhz.push_back(snapshot.speedMhz);
        metrics.history.temperatureCelsius.push_back(snapshot.temperatureCelsius);
        metrics.history.userPercent.push_back(snapshot.breakdown.userPercent);
        metrics.history.kernelPercent.push_back(snapshot.breakdown.kernelPercent);
    }
    return metrics;
}

/** @brief Body of the sampling thread: one sample per interval until stop(). */
void CpuMonitorService::runSampling()
{
    auto nextTick = std::chrono::steady_clock::now() + m_options.interval;
    while (true)
    {
        {
            std::unique_lock lock(m_mutex);
            // wait_until (not sleep): stop() wakes the thread at once, so shutdown never waits an interval.
            if (m_wake.wait_until(lock, nextTick, [this] { return m_stopRequested; }))
            {
                return;
            }
        }
        sampleOnce();
        // Fixed-rate schedule; when a read took longer than an interval, restart from now instead of
        // firing a burst of catch-up samples.
        nextTick += m_options.interval;
        const auto now = std::chrono::steady_clock::now();
        if (nextTick < now)
        {
            nextTick = now + m_options.interval;
        }
    }
}

/** @brief Read the provider, compute the interval against the previous read and publish it. */
void CpuMonitorService::sampleOnce()
{
    providers::cpu::CpuCounters counters;
    try
    {
        counters = m_provider->readCounters();
    }
    catch (const std::exception& e)
    {
        logProviderError(e.what());
        return;
    }
    const auto now = std::chrono::steady_clock::now();

    if (m_previous)
    {
        const double elapsedSeconds = std::chrono::duration<double>(now - m_previousTime).count();
        std::uint32_t logicalProcessors = 1;
        {
            std::lock_guard lock(m_mutex);
            logicalProcessors = m_info ? m_info->logicalProcessors : 1;
        }
        const CpuSnapshot snapshot = computeSnapshot(*m_previous, counters, elapsedSeconds, logicalProcessors);
        appendHistory(snapshot);
    }
    m_previous = std::move(counters);
    m_previousTime = now;
    m_lastError.clear();
}

/**
 * @brief Publish a snapshot as the current one and append it to the bounded history.
 * @param snapshot Values of the last interval.
 */
void CpuMonitorService::appendHistory(const CpuSnapshot& snapshot)
{
    std::lock_guard lock(m_mutex);
    m_current = snapshot;
    m_history.push_back(snapshot);
    while (m_history.size() > m_options.historyLength)
    {
        m_history.pop_front();
    }
}

/**
 * @brief Throw when the platform has no CPU provider.
 * @throws FeatureUnavailableError
 */
void CpuMonitorService::requireProvider() const
{
    if (!m_provider)
    {
        throw FeatureUnavailableError(kNoProviderMessage);
    }
}

/**
 * @brief Log a provider failure, once per distinct message (a failing counter would otherwise log every second).
 * @param message Exception message.
 */
void CpuMonitorService::logProviderError(const std::string& message)
{
    if (message != m_lastError)
    {
        std::cerr << "[CpuMonitorService] provider error: " << message << std::endl;
        m_lastError = message;
    }
}

} // namespace systemlens::services::cpu
