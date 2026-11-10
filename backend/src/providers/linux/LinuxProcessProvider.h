/**
 * @file    LinuxProcessProvider.h
 * @brief   Linux IProcessProvider: /proc/<pid>/stat.
 */
#pragma once

#include "providers/IProcessProvider.h"

namespace systemlens::providers::linux_os {

class LinuxProcessProvider final : public IProcessProvider
{
public:
    LinuxProcessProvider();
    [[nodiscard]] std::vector<ProcessSample> readProcesses() override;
    [[nodiscard]] std::uint32_t currentProcessId() const override;

private:
    double m_secondsPerTick; ///< Length of one clock tick of /proc/<pid>/stat.
    std::uint64_t m_pageSize;
};

} // namespace systemlens::providers::linux_os
