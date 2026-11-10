/**
 * @file    LinuxProcessProvider.cpp
 * @brief   Implementation of LinuxProcessProvider.
 */
#include "providers/linux/LinuxProcessProvider.h"

#include "providers/ProviderError.h"
#include "providers/linux/ProcFs.h"

#include <unistd.h>

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
constexpr std::size_t kResidentPagesIndex = 21;
/** The kernel truncates comm to 15 characters (TASK_COMM_LEN - 1). */
constexpr std::size_t kMaxCommLength = 15;

/**
 * @brief Recover the full executable name when comm was truncated.
 * @param processDir /proc/<pid>.
 * @param comm       Name from the stat file.
 * @return Base name of argv[0] when it extends comm; otherwise comm.
 */
std::string resolveName(const std::filesystem::path& processDir, std::string_view comm)
{
    if (comm.size() == kMaxCommLength)
    {
        if (const auto commandLine = procfs::readFile(processDir / "cmdline"))
        {
            // Arguments are NUL-separated; argv[0] is everything up to the first NUL.
            std::string_view program(commandLine->c_str());
            if (const auto slash = program.rfind('/'); slash != std::string_view::npos)
            {
                program.remove_prefix(slash + 1);
            }
            if (program.starts_with(comm))
            {
                return std::string(program);
            }
        }
    }
    return std::string(comm);
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

        const auto ticks = procfs::parseUnsigned(fields[kUserTicksIndex]).value_or(0)
                           + procfs::parseUnsigned(fields[kKernelTicksIndex]).value_or(0);
        processes.push_back(ProcessSample{
            .pid = static_cast<std::uint32_t>(procfs::parseUnsigned(pidText).value_or(0)),
            .parentPid = static_cast<std::uint32_t>(procfs::parseUnsigned(fields[kParentPidIndex]).value_or(0)),
            .name = resolveName(entry.path(), comm),
            .cpuTime = std::chrono::duration<double>(static_cast<double>(ticks) * m_secondsPerTick),
            .memoryBytes = procfs::parseUnsigned(fields[kResidentPagesIndex]).value_or(0) * m_pageSize,
        });
    }
    return processes;
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
