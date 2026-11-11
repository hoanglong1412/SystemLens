/**
 * @file    WindowsProcessProvider.h
 * @brief   Windows IProcessProvider: NtQuerySystemInformation(SystemProcessInformation).
 */
#pragma once

#include "providers/IProcessProvider.h"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace systemlens::providers::windows_os {

class WindowsProcessProvider final : public IProcessProvider
{
public:
    [[nodiscard]] std::vector<ProcessSample> readProcesses() override;
    [[nodiscard]] std::uint32_t currentProcessId() const override;

private:
    std::string lookUpOwner(std::uint32_t pid);
    /** Reused between calls: the process list needs a few hundred kilobytes every second. */
    std::vector<std::max_align_t> m_buffer;
    /** PID → owner, empty when unreadable. Looking an owner up opens the process token, so it is done once per PID. */
    std::unordered_map<std::uint32_t, std::string> m_owners;
};

} // namespace systemlens::providers::windows_os
