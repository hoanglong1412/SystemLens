/**
 * @file    AppConfig.cpp
 * @brief   Implementation of parseAppConfig().
 */
#include "app/AppConfig.h"

#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace systemlens::app {

namespace {

constexpr int kMaxPort = 65535;

/**
 * @brief Read an environment variable.
 * @param name Variable name.
 * @return Its value, or std::nullopt when unset or empty.
 */
std::optional<std::string> readEnv(const char* name)
{
#ifdef _MSC_VER
    // MSVC flags std::getenv as unsafe (C4996); _dupenv_s is its recommended replacement.
    char* buffer = nullptr;
    size_t length = 0;
    if (_dupenv_s(&buffer, &length, name) != 0 || buffer == nullptr)
    {
        return std::nullopt;
    }
    std::string value(buffer);
    std::free(buffer);
#else
    const char* raw = std::getenv(name);
    if (raw == nullptr)
    {
        return std::nullopt;
    }
    std::string value(raw);
#endif
    if (value.empty())
    {
        return std::nullopt;
    }
    return value;
}

/**
 * @brief Parse a port number.
 * @param text Command-line value.
 * @return Port in [0, 65535].
 * @throws std::invalid_argument when text is not a valid port.
 */
int parsePort(std::string_view text)
{
    try
    {
        size_t consumed = 0;
        const int port = std::stoi(std::string(text), &consumed);
        if (consumed == text.size() && port >= 0 && port <= kMaxPort)
        {
            return port;
        }
    }
    catch (const std::exception&)
    {
        // Falls through to the error below.
    }
    throw std::invalid_argument("Invalid port: " + std::string(text));
}

} // namespace

/**
 * @brief Build the configuration from argv and the environment.
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 * @return Parsed configuration.
 * @throws std::invalid_argument on unknown/invalid arguments or a missing token outside dev mode.
 */
AppConfig parseAppConfig(int argc, const char* const argv[])
{
    AppConfig config;

    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];
        if (arg == "--port")
        {
            if (i + 1 >= argc)
            {
                throw std::invalid_argument("--port needs a value");
            }
            config.port = parsePort(argv[++i]);
        }
        else if (arg == "--dev")
        {
            config.devMode = true;
        }
        else if (arg == "--help" || arg == "-h")
        {
            config.showHelp = true;
        }
        else
        {
            throw std::invalid_argument("Unknown argument: " + std::string(arg));
        }
    }

    config.token = readEnv(kTokenEnvVar);
    if (!config.token && !config.devMode && !config.showHelp)
    {
        throw std::invalid_argument(std::string(kTokenEnvVar) + " is not set (use --dev to run without auth)");
    }
    return config;
}

} // namespace systemlens::app
