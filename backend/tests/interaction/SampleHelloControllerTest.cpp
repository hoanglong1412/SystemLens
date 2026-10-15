/**
 * @file    SampleHelloControllerTest.cpp
 * @brief   Interaction tests for SampleHelloController: verifies how the controller talks to
 *          the route registry and to ISampleHelloService (mocked with gmock). No HTTP involved.
 */
#include "api/ApiResponse.h"
#include "api/sample/SampleHelloController.h"
#include "services/sample/ISampleHelloService.h"
#include "support/FakeRouteRegistry.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace systemlens::tests {
namespace {

using api::HttpMethod;
using api::sample::SampleHelloController;
using ::testing::Return;
using ::testing::Throw;

/** gmock implementation of the service so the test controls and observes every call. */
class MockSampleHelloService : public services::sample::ISampleHelloService
{
public:
    MOCK_METHOD(std::string, getGreeting, (), (const, override));
};

/** Shared setup: a mocked service, the controller under test and a fake registry. */
class SampleHelloControllerTest : public ::testing::Test
{
protected:
    /** @brief Register the controller's routes on the fake registry. */
    void SetUp() override { controller.registerRoutes(registry); }

    std::shared_ptr<MockSampleHelloService> service = std::make_shared<MockSampleHelloService>();
    SampleHelloController controller{service};
    FakeRouteRegistry registry;
};

TEST_F(SampleHelloControllerTest, RegistersExactlyOneGetRoute)
{
    ASSERT_EQ(registry.routes().size(), 1u);
    EXPECT_EQ(registry.routes()[0].method, HttpMethod::Get);
    EXPECT_EQ(registry.routes()[0].path, SampleHelloController::kRoute);
}

TEST_F(SampleHelloControllerTest, CallsServiceOnceAndReturnsItsGreetingAsJson)
{
    EXPECT_CALL(*service, getGreeting()).Times(1).WillOnce(Return("hi from mock"));

    const api::Response response = registry.call(HttpMethod::Get, SampleHelloController::kRoute);

    EXPECT_EQ(response.status, api::status::kOk);
    EXPECT_EQ(response.contentType, "application/json");
    EXPECT_EQ(nlohmann::json::parse(response.body), (nlohmann::json{{"message", "hi from mock"}}));
}

TEST_F(SampleHelloControllerTest, PropagatesServiceExceptionsToTheServer)
{
    // The controller must not swallow errors: HttpServer turns them into 500 problems.
    EXPECT_CALL(*service, getGreeting()).WillOnce(Throw(std::runtime_error("boom")));

    EXPECT_THROW(registry.call(HttpMethod::Get, SampleHelloController::kRoute), std::runtime_error);
}

TEST(SampleHelloControllerConstruction, RejectsNullService)
{
    EXPECT_THROW(SampleHelloController{nullptr}, std::invalid_argument);
}

} // namespace
} // namespace systemlens::tests
