/**
 * @file    Application.cpp
 * @brief   Implementation of Application.
 */
#include "app/Application.h"

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
 * @brief Build the object graph. Does not open any socket yet.
 * @param config Parsed startup configuration.
 */
Application::Application(AppConfig config)
    : m_config(std::move(config))
    , m_server(toServerOptions(m_config))
{
    registerControllers();
}

/**
 * @brief Bind, announce readiness and serve until shutdown.
 * @return Process exit code (0 on normal shutdown).
 * @throws std::runtime_error if the server cannot bind.
 */
int Application::run()
{
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
    std::cerr << "[Application] stopped" << std::endl;
    return 0;
}

/** @brief Create every controller and register its routes on the server. */
void Application::registerControllers()
{
    // Add new controllers here: create the service(s), the controller, then register its routes.
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
