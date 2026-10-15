/**
 * @file    SampleHelloApiTest.cpp
 * @brief   API tests for GET /api/v1/sample/hello: start a real HttpServer on a random
 *          loopback port and call it over HTTP, checking the contract in api/openapi.yaml
 *          (status codes, content types, bodies) plus auth, CORS and error handling.
 */
#include "api/HttpServer.h"
#include "api/sample/SampleHelloController.h"
#include "services/sample/ISampleHelloService.h"
#include "services/sample/SampleHelloService.h"

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <stdexcept>
#include <thread>

namespace systemlens::tests {
namespace {

using api::sample::SampleHelloController;

constexpr const char* kToken = "test-token";
constexpr const char* kHost = "127.0.0.1";

/** Service that always fails, to check how the server reports unexpected errors. */
class ThrowingSampleHelloService : public services::sample::ISampleHelloService
{
public:
    std::string getGreeting() const override { throw std::runtime_error("service failure"); }
};

/**
 * Starts an HttpServer (with auth) serving SampleHelloController backed by the given
 * service, on a background thread; stops it when the fixture is destroyed.
 */
class RunningServer
{
public:
    /**
     * @brief Bind on a random port and start serving.
     * @param service Service injected into the controller.
     */
    explicit RunningServer(std::shared_ptr<const services::sample::ISampleHelloService> service)
        : m_controller(std::move(service))
        , m_server(api::HttpServerOptions{kHost, 0, kToken})
    {
        m_controller.registerRoutes(m_server);
        m_port = m_server.bind();
        m_thread = std::thread([this] { m_server.listen(); });
        m_server.waitUntilReady();
    }

    /** @brief Stop the server and wait for its thread. */
    ~RunningServer()
    {
        m_server.stop();
        m_thread.join();
    }

    /** @brief HTTP client pointed at the server. */
    httplib::Client client() const { return httplib::Client(kHost, m_port); }

private:
    SampleHelloController m_controller;
    api::HttpServer m_server;
    int m_port = 0;
    std::thread m_thread;
};

/** @brief Headers carrying a valid bearer token. */
httplib::Headers authHeaders()
{
    return {{"Authorization", std::string("Bearer ") + kToken}};
}

TEST(SampleHelloApiTest, ReturnsHelloClientWithValidToken)
{
    RunningServer server(std::make_shared<services::sample::SampleHelloService>());

    const auto res = server.client().Get(SampleHelloController::kRoute, authHeaders());

    ASSERT_TRUE(res) << "request failed";
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->get_header_value("Content-Type"), "application/json");
    EXPECT_EQ(nlohmann::json::parse(res->body), (nlohmann::json{{"message", "@@Hello world@@"}}));
}

TEST(SampleHelloApiTest, RejectsMissingToken)
{
    RunningServer server(std::make_shared<services::sample::SampleHelloService>());

    const auto res = server.client().Get(SampleHelloController::kRoute);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
    EXPECT_EQ(res->get_header_value("Content-Type"), "application/problem+json");
    EXPECT_EQ(nlohmann::json::parse(res->body)["status"], 401);
}

TEST(SampleHelloApiTest, RejectsWrongToken)
{
    RunningServer server(std::make_shared<services::sample::SampleHelloService>());

    const auto res = server.client().Get(SampleHelloController::kRoute, {{"Authorization", "Bearer wrong"}});

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST(SampleHelloApiTest, AnswersCorsPreflightWithoutToken)
{
    RunningServer server(std::make_shared<services::sample::SampleHelloService>());

    const auto res = server.client().Options(SampleHelloController::kRoute);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "*");
    EXPECT_NE(res->get_header_value("Access-Control-Allow-Headers").find("Authorization"), std::string::npos);
}

TEST(SampleHelloApiTest, ReturnsProblemForUnknownRoute)
{
    RunningServer server(std::make_shared<services::sample::SampleHelloService>());

    const auto res = server.client().Get("/api/v1/sample/does-not-exist", authHeaders());

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
    EXPECT_EQ(res->get_header_value("Content-Type"), "application/problem+json");
}

TEST(SampleHelloApiTest, ReturnsProblemWhenServiceThrows)
{
    RunningServer server(std::make_shared<ThrowingSampleHelloService>());

    const auto res = server.client().Get(SampleHelloController::kRoute, authHeaders());

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 500);
    EXPECT_EQ(res->get_header_value("Content-Type"), "application/problem+json");
    EXPECT_EQ(nlohmann::json::parse(res->body)["detail"], "service failure");
}

} // namespace
} // namespace systemlens::tests
