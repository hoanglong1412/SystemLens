/**
 * @file    LinuxProcessProvider.h
 * @brief   Linux IProcessProvider: /proc/<pid>/stat.
 */
#pragma once

#include "providers/IProcessProvider.h"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace systemlens::providers::linux_os {

class LinuxProcessProvider final : public IProcessProvider
{
public:
    LinuxProcessProvider();
    [[nodiscard]] std::vector<ProcessSample> readProcesses() override;
    [[nodiscard]] std::uint32_t currentProcessId() const override;

private:
    std::string lookUpOwner(const std::filesystem::path& processDir);

    double m_secondsPerTick; ///< Length of one clock tick of /proc/<pid>/stat.
    std::uint64_t m_pageSize;
    /** uid → login name, filled on demand: the same few users own almost all processes. */
    std::unordered_map<unsigned, std::string> m_userNames;
};

} // namespace systemlens::providers::linux_os
