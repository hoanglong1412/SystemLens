/**
 * @file    WindowsMemoryProvider.cpp
 * @brief   Implementation of WindowsMemoryProvider.
 */
#include "providers/windows/WindowsMemoryProvider.h"

#include "providers/ProviderError.h"

#include <windows.h>

#include <psapi.h>

namespace systemlens::providers::windows_os {

/**
 * @brief Read physical memory usage and commit figures.
 * @return Usage, where used = total - available.
 * @throws ProviderError if GlobalMemoryStatusEx fails.
 */
MemoryUsage WindowsMemoryProvider::readUsage() const
{
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (!::GlobalMemoryStatusEx(&status))
    {
        throw ProviderError("GlobalMemoryStatusEx failed");
    }
    MemoryUsage usage;
    usage.totalBytes = status.ullTotalPhys;
    usage.usedBytes = status.ullTotalPhys - status.ullAvailPhys;
    usage.availableBytes = status.ullAvailPhys;

    // The page counts of GetPerformanceInfo are optional extras: without them the basic figures still show.
    PERFORMANCE_INFORMATION info{};
    info.cb = sizeof(info);
    if (::GetPerformanceInfo(&info, sizeof(info)))
    {
        usage.cachedBytes = static_cast<std::uint64_t>(info.SystemCache) * info.PageSize;
        usage.committedBytes = static_cast<std::uint64_t>(info.CommitTotal) * info.PageSize;
        usage.commitLimitBytes = static_cast<std::uint64_t>(info.CommitLimit) * info.PageSize;
    }
    return usage;
}

} // namespace systemlens::providers::windows_os
