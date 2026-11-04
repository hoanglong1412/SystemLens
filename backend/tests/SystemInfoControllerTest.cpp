/**
 * @file    SystemInfoControllerTest.cpp
 * @brief   [TEST] SystemInfoController and SystemInfoService: 200 body, 501 without provider.
 */
#include "api/system/SystemInfoController.h"

#include "api/ApiResponse.h"
#include "fakes/FakeRouteRegistry.h"
#include "services/system/SystemInfoService.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace {

namespace api = systemlens::api;
using systemlens::providers::system::ISystemInfoProvider;
using systemlens::providers::system::SystemIdentity;
using systemlens::services::system::SystemInfoService;
using systemlens::tests::FakeRouteRegistry;

/** Provider with a fixed identity. */
class FakeSystemInfoProvider final : public ISystemInfoProvider
{
public:
    /**
     * @brief Fixed identity.
     * @return Identity of the design's sample machine.
     */
    SystemIdentity getIdentity() const override
    {
        return {"DESKTOP-SYSLENS", "Windows 10 Home x64", "Version 10.0.19045", "ASUS", "windows"};
    }
};

/**
 * @brief GET /api/v1/system/info through a controller over a real SystemInfoService.
 * @param provider Provider (may be empty).
 * @return Response.
 */
api::Response getInfo(std::shared_ptr<const ISystemInfoProvider> provider)
{
    FakeRouteRegistry registry;
    api::system::SystemInfoController controller(std::make_shared<SystemInfoService>(std::move(provider)));
    controller.registerRoutes(registry);
    return registry.get(api::system::SystemInfoController::kRoute);
}

TEST(SystemInfoControllerTest, RejectsNullService)
{
    EXPECT_THROW(api::system::SystemInfoController(nullptr), std::invalid_argument);
}

TEST(SystemInfoControllerTest, ReturnsIdentity)
{
    const auto response = getInfo(std::make_shared<FakeSystemInfoProvider>());
    ASSERT_EQ(response.status, api::status::kOk);
    const auto body = nlohmann::json::parse(response.body);
    EXPECT_EQ(body["hostName"], "DESKTOP-SYSLENS");
    EXPECT_EQ(body["osName"], "Windows 10 Home x64");
    EXPECT_EQ(body["osVersion"], "Version 10.0.19045");
    EXPECT_EQ(body["userName"], "ASUS");
    EXPECT_EQ(body["platform"], "windows");
}

TEST(SystemInfoControllerTest, WithoutProviderIs501)
{
    const auto response = getInfo(nullptr);
    EXPECT_EQ(response.status, api::status::kNotImplemented);
    EXPECT_EQ(response.contentType, "application/problem+json");
}

} // namespace
