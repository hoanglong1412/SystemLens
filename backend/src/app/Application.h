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
    /**
     * @brief Build the object graph. Does not open any socket yet.
     * @param config Parsed startup configuration.
     */
    explicit Application(AppConfig config);

    /**
     * @brief Bind, announce readiness and serve until shutdown.
     * @return Process exit code (0 on normal shutdown).
     * @throws std::runtime_error if the server cannot bind.
     */
    int run();

private:
    /** @brief Create every controller and register its routes on the server. */
    void registerControllers();

    /**
     * @brief Print the READY handshake line to stdout.
     * @param port Bound port.
     */
    void announceReady(int port) const;

    /** @brief Start the thread that stops the server when stdin closes (parent process exited). */
    void startParentWatchdog();

    AppConfig m_config;
    // Declared before m_server so they are destroyed after it: the server's handlers point into them.
    std::vector<std::unique_ptr<api::IController>> m_controllers;
    api::HttpServer m_server;
    std::thread m_parentWatchdog;
};

} // namespace systemlens::app
