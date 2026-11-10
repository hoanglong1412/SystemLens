/**
 * @file    ISystemInfoProvider.h
 * @brief   Provider interface for machine identity, boot time and system-wide object counts.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace systemlens::providers {

/** Who and what the machine is. Constant while the backend runs. */
struct SystemIdentity
{
    std::string hostName;
    std::string osName;       ///< Product name, e.g. "Windows 10 Home", "Ubuntu 24.04.1 LTS".
    std::string osVersion;    ///< Windows: major.minor.build; Linux: kernel release.
    std::string architecture; ///< e.g. "x64", "arm64".
    std::string userName;     ///< User the backend runs as.
    std::string sessionName;  ///< Windows: window station name; Linux: session type.
};

/** System-wide object counts at one instant. */
struct SystemCounts
{
    std::uint32_t processes = 0;
    std::uint32_t threads = 0;
    std::optional<std::uint64_t> handles;  ///< Windows: handles; Linux: allocated file handles.
    std::optional<std::uint32_t> services; ///< Installed services; nullopt when not countable.
};

class ISystemInfoProvider
{
public:
    virtual ~ISystemInfoProvider() = default;

    /**
     * @brief Read the machine identity.
     * @return Identity; fields the platform cannot provide are empty strings.
     * @throws ProviderError if the operating system cannot be queried at all.
     */
    [[nodiscard]] virtual SystemIdentity readIdentity() const = 0;

    /**
     * @brief Read when the operating system booted.
     * @return Boot time.
     * @throws ProviderError on failure.
     */
    [[nodiscard]] virtual std::chrono::system_clock::time_point readBootTime() const = 0;

    /**
     * @brief Read the current process / thread / handle / service counts.
     * @return Counts at the time of the call.
     * @throws ProviderError on failure.
     */
    [[nodiscard]] virtual SystemCounts readCounts() const = 0;
};

} // namespace systemlens::providers
