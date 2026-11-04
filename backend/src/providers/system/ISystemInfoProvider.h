/**
 * @file    ISystemInfoProvider.h
 * @brief   Provider interface implemented by the platform plugin: identity of the machine
 *          behind GET /api/v1/system/info.
 *
 * Part of the plugin ABI (see PluginAbi.h): any change to this file requires bumping kPluginAbiVersion.
 */
#pragma once

#include <string>

namespace systemlens::providers::system {

/** Who and what the backend runs on. A value the platform cannot report is an empty string. */
struct SystemIdentity
{
    std::string hostName;  ///< Computer / host name.
    std::string osName;    ///< e.g. "Windows 10 Home x64", "Ubuntu 24.04.1 LTS".
    std::string osVersion; ///< e.g. "Version 10.0.19045", "Kernel 6.8.0-45-generic".
    std::string userName;  ///< User the backend runs as.
    std::string platform;  ///< "windows", "linux" or "macos".
};

class ISystemInfoProvider
{
public:
    virtual ~ISystemInfoProvider() = default;

    /**
     * @brief Read the identity of the machine.
     * @return Identity; unknown values are empty strings.
     * @throws std::exception when the OS cannot be queried at all.
     */
    virtual SystemIdentity getIdentity() const = 0;
};

} // namespace systemlens::providers::system
