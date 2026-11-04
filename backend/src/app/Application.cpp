/**
 * @file    Application.cpp
 * @brief   Implementation of Application.
 */
#include "app/Application.h"

#include "platform/ExecutablePath.h"

#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

#include "api/cpu/CpuController.h"
#include "api/system/SystemInfoController.h"
#include "services/system/SystemInfoService.h"

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

/**
 * @brief Absolute path of the platform library, next to the executable.
 * @return Executable directory + file name of the systemlens_platform target.
 * @throws std::runtime_error when the executable directory cannot be determined.
 */
std::filesystem::path platformLibraryPath()
{
    // The file name comes from CMake ($<TARGET_FILE_NAME:systemlens_platform>), so it always matches
    // the built file; only the executable's directory is searched (spec platform-plugin).
    return platform::executableDirectory() / SYSTEMLENS_PLATFORM_LIBRARY_FILE;
}

} // namespace

/**
 * @brief Build the object graph. Does not open any socket yet.
 * @param config Parsed startup configuration.
 * @throws std::runtime_error when the platform library is missing, cannot be loaded or has another ABI version.
 */
Application::Application(AppConfig config)
    : m_config(std::move(config))
    , m_platformPlugin(platformLibraryPath())
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
    // Started before READY so the first metrics are ready about one interval after the frontend connects.
    m_cpuMonitor->start();
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
    m_cpuMonitor->stop();
    std::cerr << "[Application] stopped" << std::endl;
    return 0;
}

/** @brief Create every controller and register its routes on the server. */
void Application::registerControllers()
{
    // Add new controllers here: create the service(s), the controller, then register its routes.
#if SYSTEMLENS_ENABLE_SAMPLE_API
    m_controllers.push_back(std::make_unique<api::sample::SampleHelloController>(
        std::make_shared<services::sample::SampleHelloService>(m_platformPlugin.createHelloProvider())));
#endif

    m_cpuMonitor = std::make_shared<services::cpu::CpuMonitorService>(m_platformPlugin.createCpuProvider());
    m_controllers.push_back(std::make_unique<api::cpu::CpuController>(m_cpuMonitor));

    m_controllers.push_back(std::make_unique<api::system::SystemInfoController>(
        std::make_shared<services::system::SystemInfoService>(m_platformPlugin.createSystemInfoProvider())));

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
