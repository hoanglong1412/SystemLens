/**
 * @file    IHelloProvider.h
 * @brief   [SAMPLE API] Provider interface implemented by the platform plugin: supplies the
 *          platform-specific greeting behind GET /api/v1/sample/hello.
 *
 * Part of the plugin ABI (see PluginAbi.h): any change to this interface requires bumping
 * kPluginAbiVersion.
 */
#pragma once

#include <string>

namespace systemlens::providers::sample {

class IHelloProvider
{
public:
    virtual ~IHelloProvider() = default;

    /**
     * @brief Get the greeting that identifies the platform.
     * @return Greeting text, e.g. "hello from Windows".
     * @throws std::exception on failure (caught by the caller on the host side).
     */
    virtual std::string getGreeting() const = 0;
};

} // namespace systemlens::providers::sample
