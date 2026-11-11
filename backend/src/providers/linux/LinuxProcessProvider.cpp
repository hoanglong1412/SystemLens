/**
 * @file    LinuxProcessProvider.cpp
 * @brief   Implementation of LinuxProcessProvider.
 */
#include "providers/linux/LinuxProcessProvider.h"

#include "providers/ProviderError.h"
#include "providers/linux/ProcFs.h"

#include <sys/stat.h>
#include <unistd.h>

#include <pwd.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <optional>
#include <string_view>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kProcDir = "/proc";
/**
 * Indexes into the fields of /proc/<pid>/stat that follow the "(comm)" field
 * (so index 0 is field 3 of proc(5), "state").
 */
constexpr std::size_t kParentPidIndex = 1;
constexpr std::size_t kUserTicksIndex = 11;
constexpr std::size_t kKernelTicksIndex = 12;
constexpr std::size_t kStateIndex = 0;
constexpr std::size_t kThreadCountIndex = 17;
constexpr std::size_t kResidentPagesIndex = 21;
/** Longest command line sent to the UI; longer ones are cut. */
constexpr std::size_t kMaxCommandLineLength = 512;
constexpr std::size_t kPasswdBufferSize = 4096;
/** The kernel truncates comm to 15 characters (TASK_COMM_LEN - 1). */
constexpr std::size_t kMaxCommLength = 15;

/**
 * @brief Recover the full executable name when comm was truncated.
 * @param comm       Name from the stat file.
 * @param rawCommand Content of /proc/<pid>/cmdline (arguments separated by NUL).
 * @return Base name of argv[0] when it extends comm; otherwise comm.
 */
std::string resolveName(std::string_view comm, const std::string& rawCommand)
{
    if (comm.size() == kMaxCommLength)
    {
        // argv[0] is everything up to the first NUL. Programs that rewrite their own argv (Chrome, Node) store
        // the whole command line in one NUL-free block, so the first space ends the program name as well.
        std::string_view program(rawCommand.c_str());
        program = program.substr(0, program.find(' '));
        if (const auto slash = program.rfind('/'); slash != std::string_view::npos)
        {
            program.remove_prefix(slash + 1);
        }
        if (program.starts_with(comm))
        {
            return std::string(program);
        }
    }
    return std::string(comm);
}

/**
 * @brief Make a command line printable: NUL separators become spaces, length is capped.
 * @param rawCommand Content of /proc/<pid>/cmdline.
 * @return Command line; empty for kernel threads.
 */
std::string toCommandLine(std::string rawCommand)
{
    std::ranges::replace(rawCommand, '\0', ' ');
    rawCommand = std::string(procfs::trim(rawCommand));
    if (rawCommand.size() > kMaxCommandLineLength)
    {
        rawCommand.resize(kMaxCommandLineLength);
    }
    return rawCommand;
}

/**
 * @brief Translate the state letter of /proc/<pid>/stat.
 * @param state First character of the state field.
 * @return Display text such as "Running" or "Sleeping".
 */
std::string toStatusText(char state)
{
    switch (state)
    {
    case 'R':
        return "Running";
    case 'S':
        return "Sleeping";
    case 'D':
        return "Disk sleep";
    case 'Z':
        return "Zombie";
    case 'T':
    case 't':
        return "Stopped";
    case 'I':
        return "Idle";
    default:
        return "Other";
    }
}

} // namespace

/**
 * @brief Cache the clock tick length and page size (constant for the life of the system).
 * @throws ProviderError if sysconf cannot report them.
 */
LinuxProcessProvider::LinuxProcessProvider()
{
    const long ticksPerSecond = ::sysconf(_SC_CLK_TCK);
    const long pageSize = ::sysconf(_SC_PAGESIZE);
    if (ticksPerSecond <= 0 || pageSize <= 0)
    {
        throw ProviderError("sysconf(_SC_CLK_TCK / _SC_PAGESIZE) failed");
    }
    m_secondsPerTick = 1.0 / static_cast<double>(ticksPerSecond);
    m_pageSize = static_cast<std::uint64_t>(pageSize);
}

/**
 * @brief Read every /proc/<pid>/stat.
 *
 * Processes that exit while the list is being read are skipped.
 * @return One sample per live process.
 * @throws ProviderError if /proc cannot be listed.
 */
std::vector<ProcessSample> LinuxProcessProvider::readProcesses()
{
    std::error_code error;
    std::filesystem::directory_iterator directory(kProcDir, error);
    if (error)
    {
        throw ProviderError("Cannot list /proc: " + error.message());
    }

    std::vector<ProcessSample> processes;
    for (const auto& entry : directory)
    {
        const std::string pidText = entry.path().filename().string();
        if (!procfs::isAllDigits(pidText))
        {
            continue;
        }
        const auto stat = procfs::readFile(entry.path() / "stat");
        if (!stat)
        {
            continue;
        }

        // Format: "pid (comm) state ppid ...". comm may contain spaces and parentheses,
        // so it is delimited by the first '(' and the LAST ')'.
        const auto open = stat->find('(');
        const auto close = stat->rfind(')');
        if (open == std::string::npos || close == std::string::npos || close < open)
        {
            continue;
        }
        const std::string_view comm = std::string_view(*stat).substr(open + 1, close - open - 1);
        const auto fields = procfs::splitWhitespace(std::string_view(*stat).substr(close + 1));
        if (fields.size() <= kResidentPagesIndex)
        {
            continue;
        }

        const std::string rawCommand = procfs::readFile(entry.path() / "cmdline").value_or("");
        const auto ticks = procfs::parseUnsigned(fields[kUserTicksIndex]).value_or(0)
                           + procfs::parseUnsigned(fields[kKernelTicksIndex]).value_or(0);
        processes.push_back(ProcessSample{
            .pid = static_cast<std::uint32_t>(procfs::parseUnsigned(pidText).value_or(0)),
            .parentPid = static_cast<std::uint32_t>(procfs::parseUnsigned(fields[kParentPidIndex]).value_or(0)),
            .name = resolveName(comm, rawCommand),
            .cpuTime = std::chrono::duration<double>(static_cast<double>(ticks) * m_secondsPerTick),
            .memoryBytes = procfs::parseUnsigned(fields[kResidentPagesIndex]).value_or(0) * m_pageSize,
            .userName = lookUpOwner(entry.path()),
            .threadCount = static_cast<std::uint32_t>(procfs::parseUnsigned(fields[kThreadCountIndex]).value_or(0)),
            .status = toStatusText(fields[kStateIndex].front()),
            .commandLine = toCommandLine(rawCommand),
        });
    }
    return processes;
}

/**
 * @brief Find the owner of a process: the owner of its /proc/<pid> directory.
 * @param processDir /proc/<pid>.
 * @return Login name, the numeric uid when it has no passwd entry, or empty when the directory vanished.
 */
std::string LinuxProcessProvider::lookUpOwner(const std::filesystem::path& processDir)
{
    struct stat info{};
    if (::stat(processDir.c_str(), &info) != 0)
    {
        return {};
    }
    const unsigned uid = info.st_uid;
    if (const auto cached = m_userNames.find(uid); cached != m_userNames.end())
    {
        return cached->second;
    }
    std::array<char, kPasswdBufferSize> buffer{};
    passwd entry{};
    passwd* result = nullptr;
    const bool found = ::getpwuid_r(uid, &entry, buffer.data(), buffer.size(), &result) == 0 && result != nullptr;
    return m_userNames.emplace(uid, found ? std::string(result->pw_name) : std::to_string(uid)).first->second;
}

/**
 * @brief PID of the backend.
 * @return getpid().
 */
std::uint32_t LinuxProcessProvider::currentProcessId() const
{
    return static_cast<std::uint32_t>(::getpid());
}

} // namespace systemlens::providers::linux_os
