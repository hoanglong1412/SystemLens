/**
 * @file    IProcessProvider.h
 * @brief   Provider interface for the process list with cumulative CPU time.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace systemlens::providers {

/** One process at one instant. */
struct ProcessSample
{
    std::uint32_t pid = 0;
    std::uint32_t parentPid = 0;
    std::string name;                        ///< Image name, e.g. "msedge.exe", "bash".
    std::chrono::duration<double> cpuTime{}; ///< Cumulative user + kernel time.
    std::uint64_t memoryBytes = 0;           ///< Windows: private working set; Linux: resident set size.
};

class IProcessProvider
{
public:
    virtual ~IProcessProvider() = default;

    /**
     * @brief Snapshot of every process visible to the backend (the idle pseudo-process excluded).
     * @return Processes in no particular order.
     * @throws ProviderError if the process list cannot be read.
     */
    [[nodiscard]] virtual std::vector<ProcessSample> readProcesses() = 0;

    /**
     * @brief Identify the backend process itself.
     * @return PID of the calling process.
     */
    [[nodiscard]] virtual std::uint32_t currentProcessId() const = 0;
};

} // namespace systemlens::providers
