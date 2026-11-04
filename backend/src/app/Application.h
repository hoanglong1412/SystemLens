/**
 * @file    Application.h
 * @brief   Composition root of the backend: creates services and controllers, wires them
 *          into the HttpServer, performs the startup handshake and runs until shutdown.
 *
 * Platform plugin: the constructor loads the platform library from the executable's directory
 * (see platform/PlatformPlugin.h). A library-level failure throws, so no READY line is printed.
 *
 * Startup handshake (consumed by frontend/main/backendProcess.js):
 *   once the socket is bound, one line is written to stdout:
 *     SYSTEMLENS_READY {"port":<port>}
 *   stdout carries nothing else; all logs go to stderr.
 *
 * Shutdown: outside dev mode the parent (Electron) keeps our stdin open. When stdin
 * reaches EOF the parent is gone, so the server stops and the process exits.
 */
#pragma once

#include "api/HttpServer.h"
#include "api/IController.h"
#include "app/AppConfig.h"
#include "platform/PlatformPlugin.h"
#include "services/cpu/CpuMonitorService.h"

#include <memory>
#include <thread>
#include <vector>

namespace systemlens::app {

/** Prefix of the stdout line announcing that the backend is ready. */
inline constexpr const char* kReadyPrefix = "SYSTEMLENS_READY ";

class Application
{
public:
    explicit Application(AppConfig config);
    int run();

private:
    void registerControllers();
    void announceReady(int port) const;
    void startParentWatchdog();

    AppConfig m_config;
    // Declared before the controllers so it is destroyed after them: services hold providers created
    // by the plugin (the provider deleters also keep the library loaded, see PlatformPlugin).
    platform::PlatformPlugin m_platformPlugin;
    // Owns the CPU sampling thread; started in run(), stopped after the server stops.
    std::shared_ptr<services::cpu::CpuMonitorService> m_cpuMonitor;
    // Declared before m_server so they are destroyed after it: the server's handlers point into them.
    std::vector<std::unique_ptr<api::IController>> m_controllers;
    api::HttpServer m_server;
    std::thread m_parentWatchdog;
};

} // namespace systemlens::app
