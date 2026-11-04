/**
 * @file    CpuControllerTest.cpp
 * @brief   [TEST] CpuController and CpuJson: routes, JSON shape (null fields), 200 / 501 / 503.
 */
#include "api/cpu/CpuController.h"

#include "api/ApiResponse.h"
#include "api/cpu/CpuJson.h"
#include "fakes/FakeRouteRegistry.h"
#include "services/FeatureUnavailableError.h"
#include "services/NotReadyError.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace {

namespace api = systemlens::api;
namespace cpu = systemlens::services::cpu;
using systemlens::providers::cpu::CpuStaticInfo;
using systemlens::providers::cpu::Virtualization;
using systemlens::tests::FakeRouteRegistry;

/** What the fake service does on each call. */
enum class Behavior
{
    Ok,
    Unavailable,
    NotReady,
};

/** ICpuMonitorService with fixed data or a fixed error. */
class FakeCpuMonitorService final : public cpu::ICpuMonitorService
{
public:
    /**
     * @brief Create the fake.
     * @param behavior Data or error returned by every call.
     */
    explicit FakeCpuMonitorService(Behavior behavior)
        : m_behavior(behavior)
    {
    }

    /**
     * @brief Fixed static info.
     * @return Info of a 4-core / 8-thread CPU.
     * @throws FeatureUnavailableError / NotReadyError depending on the behavior.
     */
    CpuStaticInfo getInfo() const override
    {
        fail();
        CpuStaticInfo info;
        info.model = "Test CPU";
        info.sockets = 1;
        info.cores = 4;
        info.logicalProcessors = 8;
        info.baseSpeedMhz = 2300;
        info.virtualization = Virtualization::Enabled;
        info.l3CacheBytes = 8u * 1024 * 1024;
        info.countersSource = "test";
        return info;
    }

    /**
     * @brief Fixed metrics with some missing values.
     * @return Metrics.
     * @throws FeatureUnavailableError / NotReadyError depending on the behavior.
     */
    cpu::CpuMetrics getMetrics() const override
    {
        fail();
        cpu::CpuMetrics m;
        m.sampleIntervalMs = 1000;
        m.historyLength = 60;
        m.current.usagePercent = 12.3456;
        m.current.perCoreUsagePercent = {10, 20};
        m.current.speedMhz = 2610;
        m.current.processes = 253;
        m.current.bootTimeUnixSeconds = 0;
        m.current.scheduler.contextSwitchesPerSec = 24318;
        m.current.topProcesses = {{4, "System", 0.19, 4096}};
        m.history.usagePercent = {1, 2};
        m.history.speedMhz = {2600, std::nullopt};
        m.history.temperatureCelsius = {std::nullopt, std::nullopt};
        m.history.userPercent = {1, 1};
        m.history.kernelPercent = {0, 1};
        return m;
    }

private:
    /**
     * @brief Throw the error of the configured behavior, if any.
     * @throws FeatureUnavailableError / NotReadyError
     */
    void fail() const
    {
        if (m_behavior == Behavior::Unavailable)
        {
            throw systemlens::services::FeatureUnavailableError("no cpu here");
        }
        if (m_behavior == Behavior::NotReady)
        {
            throw systemlens::services::NotReadyError("not yet");
        }
    }

    Behavior m_behavior;
};

/**
 * @brief Register a controller backed by the fake service.
 * @param behavior Fake behavior.
 * @param registry Out: receives the routes.
 * @return The controller (must outlive the registry calls).
 */
std::unique_ptr<api::cpu::CpuController> makeController(Behavior behavior, FakeRouteRegistry& registry)
{
    auto controller = std::make_unique<api::cpu::CpuController>(std::make_shared<FakeCpuMonitorService>(behavior));
    controller->registerRoutes(registry);
    return controller;
}

TEST(CpuControllerTest, RejectsNullService)
{
    EXPECT_THROW(api::cpu::CpuController(nullptr), std::invalid_argument);
}

TEST(CpuControllerTest, RegistersBothRoutes)
{
    FakeRouteRegistry registry;
    const auto controller = makeController(Behavior::Ok, registry);
    EXPECT_TRUE(registry.has(api::HttpMethod::Get, "/api/v1/cpu/info"));
    EXPECT_TRUE(registry.has(api::HttpMethod::Get, "/api/v1/cpu/metrics"));
}

TEST(CpuControllerTest, InfoBody)
{
    FakeRouteRegistry registry;
    const auto controller = makeController(Behavior::Ok, registry);
    const auto response = registry.get(api::cpu::CpuController::kInfoRoute);
    ASSERT_EQ(response.status, api::status::kOk);
    EXPECT_EQ(response.contentType, "application/json");

    const auto body = nlohmann::json::parse(response.body);
    EXPECT_EQ(body["model"], "Test CPU");
    EXPECT_EQ(body["logicalProcessors"], 8);
    EXPECT_EQ(body["virtualization"], "enabled");
    EXPECT_EQ(body["baseSpeedMhz"], 2300.0);
    EXPECT_TRUE(body["maxSpeedMhz"].is_null());
    EXPECT_TRUE(body["cache"]["l1Bytes"].is_null());
    EXPECT_EQ(body["cache"]["l3Bytes"], 8u * 1024 * 1024);
}

TEST(CpuControllerTest, MetricsBodyKeepsNullFieldsAndRounds)
{
    FakeRouteRegistry registry;
    const auto controller = makeController(Behavior::Ok, registry);
    const auto response = registry.get(api::cpu::CpuController::kMetricsRoute);
    ASSERT_EQ(response.status, api::status::kOk);

    const auto body = nlohmann::json::parse(response.body);
    const auto& current = body["current"];
    EXPECT_EQ(current["usagePercent"], 12.35);
    EXPECT_EQ(current["perCoreUsagePercent"].size(), 2u);
    EXPECT_TRUE(current.contains("temperatureCelsius"));
    EXPECT_TRUE(current["temperatureCelsius"].is_null());
    EXPECT_TRUE(current["handles"].is_null());
    EXPECT_TRUE(current["scheduler"]["systemCallsPerSec"].is_null());
    EXPECT_EQ(current["scheduler"]["contextSwitchesPerSec"], 24318.0);
    EXPECT_EQ(current["bootTime"], "1970-01-01T00:00:00Z");
    EXPECT_EQ(current["topProcesses"][0]["name"], "System");
    EXPECT_TRUE(body["history"]["speedMhz"][1].is_null());
    EXPECT_EQ(body["historyLength"], 60);
}

TEST(CpuControllerTest, UnavailableIs501Problem)
{
    FakeRouteRegistry registry;
    const auto controller = makeController(Behavior::Unavailable, registry);
    for (const char* route : {api::cpu::CpuController::kInfoRoute, api::cpu::CpuController::kMetricsRoute})
    {
        const auto response = registry.get(route);
        EXPECT_EQ(response.status, api::status::kNotImplemented) << route;
        EXPECT_EQ(response.contentType, "application/problem+json");
        EXPECT_EQ(nlohmann::json::parse(response.body)["status"], 501);
    }
}

TEST(CpuControllerTest, NotReadyIs503Problem)
{
    FakeRouteRegistry registry;
    const auto controller = makeController(Behavior::NotReady, registry);
    const auto response = registry.get(api::cpu::CpuController::kMetricsRoute);
    EXPECT_EQ(response.status, api::status::kServiceUnavailable);
    EXPECT_EQ(nlohmann::json::parse(response.body)["title"], "Service Unavailable");
}

TEST(CpuJsonTest, Iso8601Utc)
{
    EXPECT_EQ(api::cpu::toIso8601Utc(0), "1970-01-01T00:00:00Z");
    EXPECT_EQ(api::cpu::toIso8601Utc(1789373561), "2026-09-14T08:12:41Z");
    EXPECT_EQ(api::cpu::toIso8601Utc(-1), "1969-12-31T23:59:59Z");
}

} // namespace
