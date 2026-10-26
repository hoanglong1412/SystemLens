/**
 * @file    Application.h
 * @brief   Composition root of the backend: creates services and controllers, wires them
 *          into the HttpServer, performs the startup handshake and runs until shutdown.
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
    // Declared before m_server so they are destroyed after it: the server's handlers point into them.
    std::vector<std::unique_ptr<api::IController>> m_controllers;
    api::HttpServer m_server;
    std::thread m_parentWatchdog;
};

} // namespace systemlens::app
