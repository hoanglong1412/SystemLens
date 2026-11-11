/**
 * @file    ISystemServiceProvider.h
 * @brief   Provider interface for the operating system services (Windows services, systemd units).
 *
 * Named ISystemServiceProvider because Windows already defines a COM type called IServiceProvider.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::providers {

/** Run state of a service. */
enum class ServiceState
{
    Running,
    Stopped,
    Starting,
    Stopping,
    Paused,
    Other,
};

/** When a service is started. */
enum class ServiceStartType
{
    Automatic,
    Manual,
    Disabled,
    Static, ///< Started only as a dependency of something else (systemd "static").
    Other,
};

/** One service. */
struct SystemServiceInfo
{
    std::string name;
    std::string displayName;
    std::optional<std::string> description;
    ServiceState state = ServiceState::Other;
    ServiceStartType startType = ServiceStartType::Other;
    std::optional<std::uint32_t> pid; ///< Windows only.
};

class ISystemServiceProvider
{
public:
    virtual ~ISystemServiceProvider() = default;

    /**
     * @brief List the installed services. Stateless and safe to call from several threads.
     * @return Services in no particular order.
     * @throws ProviderError if the platform cannot list services (e.g. no systemd).
     */
    [[nodiscard]] virtual std::vector<SystemServiceInfo> readServices() const = 0;
};

} // namespace systemlens::providers
