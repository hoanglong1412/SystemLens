/**
 * @file    Application.cpp
 * @brief   Implementation of Application.
 */
#include "app/Application.h"

#include "api/inventory/InventoryController.h"
#include "api/performance/PerformanceController.h"
#include "api/performance/PerformanceDetailController.h"
#include "api/processes/ProcessController.h"
#include "api/system/SystemController.h"
#include "providers/PlatformProviders.h"
#include "services/inventory/SystemInventoryService.h"
#include "services/performance/PerformanceDetailService.h"
#include "services/performance/PerformanceService.h"
#include "services/processes/ProcessListService.h"
#include "services/system/SystemOverviewService.h"

#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

#if SYSTEMLENS_ENABLE_SAMPLE_API
#include "api/sample/SampleHelloController.h"
#include "services/sample/SampleHelloService.h"
#endif

namespace systemlens::app {

namespace {

/**
 * @brief Translate the app configuration into HTTP server options.
 * @param config Startup configuration.
 * @return Options binding to loopback, with auth disabled only in dev mode.
 */
api::HttpServerOptions toServerOptions(const AppConfig& config)
{
    api::HttpServerOptions options;
    options.port = config.port;
    options.token = config.devMode ? std::nullopt : config.token;
    return options;
}

} // namespace

/**
 * @brief Build the object graph. Does not open any socket or start any thread yet.
 * @param config Parsed startup configuration.
 * @throws providers::ProviderError if a platform provider cannot initialize.
 */
Application::Application(AppConfig config)
    : m_config(std::move(config))
    , m_providers(providers::createPlatformProviders())
    , m_metrics(std::make_shared<services::monitoring::MetricsCollector>(m_providers))
    , m_server(toServerOptions(m_config))
{
    registerControllers();
}

/**
 * @brief Start sampling, bind, announce readiness and serve until shutdown.
 * @return Process exit code (0 on normal shutdown).
 * @throws std::runtime_error if the server cannot bind.
 */
int Application::run()
{
    // Sampling starts before READY so the first API call already finds a snapshot.
    m_metrics->start();
    const int port = m_server.bind();

    if (m_config.devMode)
    {
        std::cerr << "[Application] WARNING: --dev mode, authentication is DISABLED" << std::endl;
    }
    else
    {
        startParentWatchdog();
    }

    announceReady(port);
    m_server.listen();

    if (m_parentWatchdog.joinable())
    {
        // listen() only returns after stop(), which in production is called by the watchdog itself,
        // so the thread has already finished reading stdin and join() returns immediately.
        m_parentWatchdog.join();
    }
    m_metrics->stop();
    std::cerr << "[Application] stopped" << std::endl;
    return 0;
}

/** @brief Create every controller and register its routes on the server. */
void Application::registerControllers()
{
    // Add new controllers here: create the service(s), the controller, then register its routes.
    m_controllers.push_back(std::make_unique<api::system::SystemController>(
        std::make_shared<services::system::SystemOverviewService>(m_metrics)));
    m_controllers.push_back(std::make_unique<api::performance::PerformanceController>(
        std::make_shared<services::performance::PerformanceService>(m_metrics)));
    m_controllers.push_back(std::make_unique<api::performance::PerformanceDetailController>(
        std::make_shared<services::performance::PerformanceDetailService>(m_metrics)));
    m_controllers.push_back(std::make_unique<api::processes::ProcessController>(
        std::make_shared<services::processes::ProcessListService>(m_metrics)));
    m_controllers.push_back(std::make_unique<api::inventory::InventoryController>(
        std::make_shared<services::inventory::SystemInventoryService>(
            m_providers.services, m_providers.startup, m_providers.sessions, m_metrics)));
#if SYSTEMLENS_ENABLE_SAMPLE_API
    m_controllers.push_back(std::make_unique<api::sample::SampleHelloController>(
        std::make_shared<services::sample::SampleHelloService>()));
#endif

    for (const auto& controller : m_controllers)
    {
        controller->registerRoutes(m_server);
    }
}

/**
 * @brief Print the READY handshake line to stdout.
 * @param port Bound port.
 */
void Application::announceReady(int port) const
{
    // std::endl flushes: the parent waits for this exact line before using the API.
    std::cout << kReadyPrefix << nlohmann::json{{"port", port}}.dump() << std::endl;
    std::cerr << "[Application] listening on http://127.0.0.1:" << port << std::endl;
}

/** @brief Start the thread that stops the server when stdin closes (parent process exited). */
void Application::startParentWatchdog()
{
    m_parentWatchdog = std::thread([this] {
        // Blocks until the parent closes our stdin (normal quit) or dies (OS closes the pipe).
        std::string ignored;
        while (std::getline(std::cin, ignored))
        {
        }
        std::cerr << "[Application] stdin closed, shutting down" << std::endl;
        m_server.stop();
    });
}

} // namespace systemlens::app
