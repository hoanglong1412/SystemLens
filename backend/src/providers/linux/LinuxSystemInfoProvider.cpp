/**
 * @file    LinuxSystemInfoProvider.cpp
 * @brief   Implementation of LinuxSystemInfoProvider.
 */
#include "providers/linux/LinuxSystemInfoProvider.h"

#include "providers/ProviderError.h"
#include "providers/linux/ProcFs.h"

#include <pwd.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <string_view>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kOsReleaseFile = "/etc/os-release";
constexpr const char* kProcStatFile = "/proc/stat";
constexpr const char* kProcDir = "/proc";
constexpr const char* kLoadAvgFile = "/proc/loadavg";
constexpr const char* kFileNrFile = "/proc/sys/fs/file-nr";
constexpr std::string_view kPrettyNameKey = "PRETTY_NAME=";
constexpr std::string_view kBootTimeKey = "btime ";
constexpr const char* kSessionTypeEnvVar = "XDG_SESSION_TYPE";
constexpr const char* kDefaultSessionName = "console";
constexpr const char* kDefaultOsName = "Linux";
/** Position of the "running/total" token in /proc/loadavg. */
constexpr std::size_t kLoadAvgEntitiesIndex = 3;
constexpr std::size_t kHostNameBufferSize = 256;
constexpr std::size_t kPasswdBufferSize = 4096;

/**
 * @brief Read the distribution name from /etc/os-release.
 * @return PRETTY_NAME without quotes, or "Linux" when the file or key is missing.
 */
std::string readOsName()
{
    const auto content = procfs::readFile(kOsReleaseFile);
    if (content)
    {
        for (std::string_view line : procfs::splitLines(*content))
        {
            if (line.starts_with(kPrettyNameKey))
            {
                line.remove_prefix(kPrettyNameKey.size());
                line = procfs::trim(line);
                if (line.size() >= 2 && line.front() == '"' && line.back() == '"')
                {
                    line = line.substr(1, line.size() - 2);
                }
                return std::string(line);
            }
        }
    }
    return kDefaultOsName;
}

/**
 * @brief Translate the uname machine name to the short names the UI uses.
 * @param machine uname -m value, e.g. "x86_64".
 * @return "x64", "arm64", or machine unchanged.
 */
std::string toArchitectureName(std::string_view machine)
{
    if (machine == "x86_64" || machine == "amd64")
    {
        return "x64";
    }
    if (machine == "aarch64")
    {
        return "arm64";
    }
    return std::string(machine);
}

/**
 * @brief Read the name of the user the backend runs as.
 * @return Login name, or the numeric uid when it has no passwd entry (e.g. in containers).
 */
std::string readUserName()
{
    const uid_t uid = ::geteuid();
    std::array<char, kPasswdBufferSize> buffer{};
    passwd entry{};
    passwd* result = nullptr;
    if (::getpwuid_r(uid, &entry, buffer.data(), buffer.size(), &result) == 0 && result != nullptr)
    {
        return result->pw_name;
    }
    return std::to_string(uid);
}

/**
 * @brief Count the processes: one numeric directory per PID in /proc.
 * @return Number of processes.
 */
std::uint32_t countProcesses()
{
    std::uint32_t count = 0;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(kProcDir, error))
    {
        if (procfs::isAllDigits(entry.path().filename().string()))
        {
            ++count;
        }
    }
    return count;
}

/**
 * @brief Read the number of threads from /proc/loadavg ("0.1 0.2 0.3 2/1234 5678").
 * @return Total scheduling entities (the number after the slash); 0 when unreadable.
 */
std::uint32_t readThreadCount()
{
    const auto content = procfs::readFile(kLoadAvgFile);
    if (!content)
    {
        return 0;
    }
    const auto tokens = procfs::splitWhitespace(*content);
    if (tokens.size() <= kLoadAvgEntitiesIndex)
    {
        return 0;
    }
    const std::string_view entities = tokens[kLoadAvgEntitiesIndex];
    const auto slash = entities.find('/');
    if (slash == std::string_view::npos)
    {
        return 0;
    }
    return static_cast<std::uint32_t>(procfs::parseUnsigned(entities.substr(slash + 1)).value_or(0));
}

/**
 * @brief Read the number of allocated file handles (first number of /proc/sys/fs/file-nr).
 * @return Handle count, or std::nullopt when unreadable.
 */
std::optional<std::uint64_t> readFileHandleCount()
{
    const auto content = procfs::readFile(kFileNrFile);
    if (!content)
    {
        return std::nullopt;
    }
    const auto tokens = procfs::splitWhitespace(*content);
    return tokens.empty() ? std::nullopt : procfs::parseUnsigned(tokens.front());
}

} // namespace

/**
 * @brief Read host name, distribution, kernel, architecture, user and session type.
 * @return Identity.
 * @throws ProviderError if uname() fails.
 */
SystemIdentity LinuxSystemInfoProvider::readIdentity() const
{
    utsname kernel{};
    if (::uname(&kernel) != 0)
    {
        throw ProviderError("uname() failed");
    }

    std::array<char, kHostNameBufferSize> hostName{};
    if (::gethostname(hostName.data(), hostName.size() - 1) != 0)
    {
        hostName.front() = '\0';
    }

    const char* sessionType = std::getenv(kSessionTypeEnvVar);
    return SystemIdentity{
        .hostName = hostName.data(),
        .osName = readOsName(),
        .osVersion = kernel.release,
        .architecture = toArchitectureName(kernel.machine),
        .userName = readUserName(),
        .sessionName = (sessionType != nullptr && *sessionType != '\0') ? sessionType : kDefaultSessionName,
    };
}

/**
 * @brief Read the boot time from the "btime" line of /proc/stat (seconds since the epoch).
 * @return Boot time.
 * @throws ProviderError if /proc/stat is unreadable or has no btime line.
 */
std::chrono::system_clock::time_point LinuxSystemInfoProvider::readBootTime() const
{
    const std::string content = procfs::readFileOrThrow(kProcStatFile);
    for (const std::string_view line : procfs::splitLines(content))
    {
        if (line.starts_with(kBootTimeKey))
        {
            if (const auto seconds = procfs::parseUnsigned(procfs::trim(line.substr(kBootTimeKey.size()))))
            {
                return std::chrono::system_clock::time_point(std::chrono::seconds(*seconds));
            }
        }
    }
    throw ProviderError("No btime line in /proc/stat");
}

/**
 * @brief Read the process, thread and file handle counts.
 * @return Counts; services is always nullopt (no init-system independent way to count them).
 */
SystemCounts LinuxSystemInfoProvider::readCounts() const
{
    return SystemCounts{
        .processes = countProcesses(),
        .threads = readThreadCount(),
        .handles = readFileHandleCount(),
        .services = std::nullopt,
    };
}

} // namespace systemlens::providers::linux_os
