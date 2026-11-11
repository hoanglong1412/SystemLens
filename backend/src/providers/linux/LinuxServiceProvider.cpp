/**
 * @file    LinuxServiceProvider.cpp
 * @brief   Implementation of LinuxServiceProvider.
 */
#include "providers/linux/LinuxServiceProvider.h"

#include "providers/ProviderError.h"
#include "providers/linux/ProcFs.h"

#include <array>
#include <cstdio>
#include <map>
#include <memory>
#include <string_view>

namespace systemlens::providers::linux_os {

namespace {

// Fixed command lines: nothing from the network or the UI is ever put into them.
constexpr const char* kListUnitsCommand =
    "systemctl list-units --type=service --all --no-legend --plain --no-pager 2>/dev/null";
constexpr const char* kListUnitFilesCommand =
    "systemctl list-unit-files --type=service --no-legend --plain --no-pager 2>/dev/null";
constexpr std::string_view kServiceSuffix = ".service";
constexpr std::size_t kReadChunkBytes = 4096;
/** Columns of list-units: UNIT LOAD ACTIVE SUB DESCRIPTION...; of list-unit-files: UNIT STATE [PRESET]. */
constexpr std::size_t kUnitColumn = 0;
constexpr std::size_t kActiveColumn = 2;
constexpr std::size_t kSubColumn = 3;
constexpr std::size_t kDescriptionFirstColumn = 4;
constexpr std::size_t kUnitFileStateColumn = 1;

/** Closes a popen stream; deleter of PipeHandle. */
struct PipeCloser
{
    void operator()(std::FILE* pipe) const noexcept { ::pclose(pipe); }
};
using PipeHandle = std::unique_ptr<std::FILE, PipeCloser>;

/**
 * @brief Run a fixed shell command and capture its output.
 * @param command Command line (a constant of this file).
 * @return Standard output.
 * @throws ProviderError if the command cannot be started.
 */
std::string runCommand(const char* command)
{
    const PipeHandle pipe(::popen(command, "r"));
    if (!pipe)
    {
        throw ProviderError("Cannot start systemctl");
    }
    std::string output;
    std::array<char, kReadChunkBytes> chunk{};
    while (const std::size_t bytes = std::fread(chunk.data(), 1, chunk.size(), pipe.get()))
    {
        output.append(chunk.data(), bytes);
    }
    return output;
}

/**
 * @param name Unit name such as "ssh.service".
 * @return The name without the ".service" suffix.
 */
std::string stripSuffix(std::string_view name)
{
    if (name.ends_with(kServiceSuffix))
    {
        name.remove_suffix(kServiceSuffix.size());
    }
    return std::string(name);
}

/**
 * @brief Translate systemd's active / sub state to a service state.
 * @param active ACTIVE column (active, inactive, failed, activating, deactivating).
 * @param sub    SUB column (running, exited, dead, ...).
 * @return Running only while the main process runs; "exited" one-shot units count as stopped.
 */
ServiceState toState(std::string_view active, std::string_view sub)
{
    if (active == "activating")
    {
        return ServiceState::Starting;
    }
    if (active == "deactivating")
    {
        return ServiceState::Stopping;
    }
    return sub == "running" ? ServiceState::Running : ServiceState::Stopped;
}

/**
 * @brief Translate a unit file state to a start type.
 * @param state STATE column of list-unit-files.
 * @return Automatic for enabled units, Disabled for disabled / masked ones, Static for units started only
 *         as dependencies.
 */
ServiceStartType toStartType(std::string_view state)
{
    if (state.starts_with("enabled") || state == "alias")
    {
        return ServiceStartType::Automatic;
    }
    if (state == "disabled" || state == "masked")
    {
        return ServiceStartType::Disabled;
    }
    if (state == "static" || state == "indirect" || state == "generated" || state == "transient")
    {
        return ServiceStartType::Static;
    }
    return ServiceStartType::Other;
}

} // namespace

/**
 * @brief List systemd services with their state and start type.
 * @return Services; units without a unit file (transient) get start type Other.
 * @throws ProviderError if systemd is not available (systemctl missing or failing).
 */
std::vector<SystemServiceInfo> LinuxServiceProvider::readServices() const
{
    const std::string units = runCommand(kListUnitsCommand);
    if (procfs::trim(units).empty())
    {
        throw ProviderError("systemd is not available (systemctl returned no units)");
    }

    std::map<std::string, ServiceStartType> startTypes;
    for (const std::string_view line : procfs::splitLines(runCommand(kListUnitFilesCommand)))
    {
        const auto columns = procfs::splitWhitespace(line);
        if (columns.size() > kUnitFileStateColumn)
        {
            startTypes.emplace(stripSuffix(columns[kUnitColumn]), toStartType(columns[kUnitFileStateColumn]));
        }
    }

    std::vector<SystemServiceInfo> services;
    for (const std::string_view line : procfs::splitLines(units))
    {
        const auto columns = procfs::splitWhitespace(line);
        if (columns.size() <= kSubColumn)
        {
            continue;
        }
        SystemServiceInfo service;
        service.name = stripSuffix(columns[kUnitColumn]);
        service.displayName = service.name;
        service.state = toState(columns[kActiveColumn], columns[kSubColumn]);
        if (const auto found = startTypes.find(service.name); found != startTypes.end())
        {
            service.startType = found->second;
        }
        // The description is the rest of the line: everything after the first four columns.
        std::string description;
        for (std::size_t column = kDescriptionFirstColumn; column < columns.size(); ++column)
        {
            description += (description.empty() ? "" : " ");
            description += columns[column];
        }
        if (!description.empty())
        {
            service.description = std::move(description);
        }
        services.push_back(std::move(service));
    }
    return services;
}

} // namespace systemlens::providers::linux_os
