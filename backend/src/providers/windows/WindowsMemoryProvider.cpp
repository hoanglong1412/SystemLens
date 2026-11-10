/**
 * @file    WindowsMemoryProvider.cpp
 * @brief   Implementation of WindowsMemoryProvider.
 */
#include "providers/windows/WindowsMemoryProvider.h"

#include "providers/ProviderError.h"

#include <windows.h>

namespace systemlens::providers::windows_os {

/**
 * @brief Read total and used physical memory.
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
    return MemoryUsage{.totalBytes = status.ullTotalPhys, .usedBytes = status.ullTotalPhys - status.ullAvailPhys};
}

} // namespace systemlens::providers::windows_os
