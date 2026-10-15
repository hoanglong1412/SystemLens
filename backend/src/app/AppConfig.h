/**
 * @file    AppConfig.h
 * @brief   Backend startup configuration parsed from the command line and environment.
 *
 * Usage: systemlens-backend.exe [--port <n>] [--dev] [--help]
 *   --port <n>  Port to listen on; 0 (default) lets the OS choose.
 *   --dev       Disable authentication (local development / curl only).
 * Environment:
 *   SYSTEMLENS_TOKEN  Bearer token required on every request (mandatory unless --dev).
 *                     Passed via environment, not argv, so it does not show up in process lists.
 */
#pragma once

#include <optional>
#include <string>

namespace systemlens::app {

struct AppConfig
{
    int port = 0;                     ///< 0 = OS picks a free port.
    std::optional<std::string> token; ///< Required bearer token; empty only in dev mode.
    bool devMode = false;             ///< True when --dev was given (auth disabled).
    bool showHelp = false;            ///< True when --help was given.
};

/** Name of the environment variable that carries the bearer token. */
inline constexpr const char* kTokenEnvVar = "SYSTEMLENS_TOKEN";

/** Help text printed for --help or invalid arguments. */
inline constexpr const char* kUsage =
    "Usage: systemlens-backend [--port <n>] [--dev] [--help]\n"
    "  --port <n>  Port to listen on (0 = any free port, default)\n"
    "  --dev       Disable authentication (development only)\n"
    "Environment: SYSTEMLENS_TOKEN = bearer token (required unless --dev)\n";

/**
 * @brief Build the configuration from argv and the environment.
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 * @return Parsed configuration.
 * @throws std::invalid_argument on unknown/invalid arguments or a missing token outside dev mode.
 */
AppConfig parseAppConfig(int argc, const char* const argv[]);

} // namespace systemlens::app
