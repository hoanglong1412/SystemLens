/**
 * @file    CpuMonitorServiceTest.cpp
 * @brief   [TEST] CpuMonitorService: missing provider, not-ready state, sampling, history cap, stop.
 *          Uses a short interval (10 ms) so the tests run in well under a second.
 */
#include "services/cpu/CpuMonitorService.h"

#include "fakes/FakeCpuProvider.h"
#include "services/FeatureUnavailableError.h"
#include "services/NotReadyError.h"

#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <stdexcept>
#include <thread>

namespace {

using namespace std::chrono_literals;
using systemlens::services::FeatureUnavailableError;
using systemlens::services::NotReadyError;
using systemlens::services::cpu::CpuMonitorOptions;
using systemlens::services::cpu::CpuMonitorService;
using systemlens::tests::FakeCpuProvider;

constexpr auto kInterval = 10ms;
constexpr auto kTimeout = 3s;

/**
 * @brief Poll a condition until it holds or the timeout expires.
 * @param condition Predicate.
 * @return true when the condition became true in time.
 */
bool waitFor(const std::function<bool()>& condition)
{
    const auto deadline = std::chrono::steady_clock::now() + kTimeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (condition())
        {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return condition();
}

/**
 * @brief Whether getMetrics() succeeds (i.e. at least one interval was measured).
 * @param service Service.
 * @return true when metrics are available.
 */
bool hasMetrics(const CpuMonitorService& service)
{
    try
    {
        service.getMetrics();
        return true;
    }
    catch (const NotReadyError&)
    {
        return false;
    }
}

TEST(CpuMonitorServiceTest, WithoutProviderEverythingIsUnavailable)
{
    CpuMonitorService service(nullptr, {kInterval, 5});
    service.start();
    EXPECT_THROW(service.getInfo(), FeatureUnavailableError);
    EXPECT_THROW(service.getMetrics(), FeatureUnavailableError);
}

TEST(CpuMonitorServiceTest, RejectsInvalidOptions)
{
    auto provider = std::make_shared<FakeCpuProvider>();
    EXPECT_THROW(CpuMonitorService(provider, {0ms, 5}), std::invalid_argument);
    EXPECT_THROW(CpuMonitorService(provider, {kInterval, 0}), std::invalid_argument);
}

TEST(CpuMonitorServiceTest, NotReadyBeforeStart)
{
    CpuMonitorService service(std::make_shared<FakeCpuProvider>(), {kInterval, 5});
    EXPECT_THROW(service.getInfo(), NotReadyError);
    EXPECT_THROW(service.getMetrics(), NotReadyError);
}

TEST(CpuMonitorServiceTest, StaticInfoFailureDisablesProvider)
{
    auto provider = std::make_shared<FakeCpuProvider>();
    provider->failStaticInfo = true;
    CpuMonitorService service(provider, {kInterval, 5});
    service.start();
    EXPECT_THROW(service.getMetrics(), FeatureUnavailableError);
}

TEST(CpuMonitorServiceTest, MeasuresIntervalsFromTheProvider)
{
    CpuMonitorService service(std::make_shared<FakeCpuProvider>(), {kInterval, 5});
    service.start();
    EXPECT_EQ(service.getInfo().logicalProcessors, FakeCpuProvider::kLogical);
    ASSERT_TRUE(waitFor([&] { return hasMetrics(service); }));

    const auto metrics = service.getMetrics();
    EXPECT_EQ(metrics.sampleIntervalMs, 10u);
    EXPECT_EQ(metrics.historyLength, 5u);
    // Fake steps: 25 user + 25 kernel + 50 idle per read → 50 % usage.
    EXPECT_NEAR(metrics.current.usagePercent, 50, 1e-9);
    EXPECT_NEAR(metrics.current.breakdown.userPercent, 25, 1e-9);
    ASSERT_EQ(metrics.current.perCoreUsagePercent.size(), FakeCpuProvider::kLogical);
    ASSERT_TRUE(metrics.current.scheduler.contextSwitchesPerSec.has_value());
    EXPECT_GT(*metrics.current.scheduler.contextSwitchesPerSec, 0);
    ASSERT_EQ(metrics.current.topProcesses.size(), 1u);
    EXPECT_EQ(metrics.current.topProcesses[0].name, "busy");
}

TEST(CpuMonitorServiceTest, HistoryIsCappedAndAligned)
{
    auto provider = std::make_shared<FakeCpuProvider>();
    CpuMonitorService service(provider, {kInterval, 3});
    service.start();
    ASSERT_TRUE(waitFor([&] { return provider->reads() >= 6; }));

    const auto h = service.getMetrics().history;
    EXPECT_EQ(h.usagePercent.size(), 3u);
    EXPECT_EQ(h.speedMhz.size(), 3u);
    EXPECT_EQ(h.temperatureCelsius.size(), 3u);
    EXPECT_EQ(h.userPercent.size(), 3u);
    EXPECT_EQ(h.kernelPercent.size(), 3u);
}

TEST(CpuMonitorServiceTest, ProviderErrorsSkipTicksButKeepSampling)
{
    auto provider = std::make_shared<FakeCpuProvider>();
    CpuMonitorService service(provider, {kInterval, 5});
    service.start();
    ASSERT_TRUE(waitFor([&] { return hasMetrics(service); }));

    provider->failReads = true;
    const auto readsBefore = provider->reads();
    std::this_thread::sleep_for(5 * kInterval);
    EXPECT_EQ(provider->reads(), readsBefore); // failing reads do not advance the fake
    EXPECT_NO_THROW(service.getMetrics());     // the last good interval stays available

    provider->failReads = false;
    EXPECT_TRUE(waitFor([&] { return provider->reads() > readsBefore + 1; }));
}

TEST(CpuMonitorServiceTest, StopIsPromptAndIdempotent)
{
    CpuMonitorService service(std::make_shared<FakeCpuProvider>(), {std::chrono::milliseconds(10'000), 5});
    service.start();
    const auto begin = std::chrono::steady_clock::now();
    service.stop();
    service.stop();
    // The sampling thread waits on a condition variable: stop() must not wait for the 10 s interval.
    EXPECT_LT(std::chrono::steady_clock::now() - begin, 1s);
}

} // namespace
