/**
 * @file    WindowsProcessProvider.h
 * @brief   Windows IProcessProvider: NtQuerySystemInformation(SystemProcessInformation).
 */
#pragma once

#include "providers/IProcessProvider.h"

#include <cstddef>

namespace systemlens::providers::windows_os {

class WindowsProcessProvider final : public IProcessProvider
{
public:
    [[nodiscard]] std::vector<ProcessSample> readProcesses() override;
    [[nodiscard]] std::uint32_t currentProcessId() const override;

private:
    /** Reused between calls: the process list needs a few hundred kilobytes every second. */
    std::vector<std::max_align_t> m_buffer;
};

} // namespace systemlens::providers::windows_os
