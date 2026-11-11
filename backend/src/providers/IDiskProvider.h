/**
 * @file    IDiskProvider.h
 * @brief   Provider interface for physical disks and their cumulative busy time.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace systemlens::providers {

/** Media type of a disk. */
enum class DiskKind
{
    Ssd,
    Hdd,
    Unknown,
};

/** One physical disk at one instant. */
struct DiskSample
{
    std::string id;   ///< Stable identifier, e.g. "disk0".
    std::string name; ///< Display name, e.g. "Disk 0 (C: E:)", "Disk 0 (nvme0n1)".
    DiskKind kind = DiskKind::Unknown;
    std::string model;                           ///< Product name; empty when unknown.
    std::optional<std::uint64_t> capacityBytes;
    std::optional<std::uint64_t> bytesRead;      ///< Cumulative.
    std::optional<std::uint64_t> bytesWritten;   ///< Cumulative.
    /** Cumulative time the disk had at least one request in flight; nullopt when not measurable. */
    std::optional<std::chrono::duration<double>> busyTime;
};

class IDiskProvider
{
public:
    virtual ~IDiskProvider() = default;

    /**
     * @brief Enumerate the physical disks with their current counters.
     * @return Disks in display order; empty when none is visible.
     * @throws ProviderError if the disks cannot be enumerated.
     */
    [[nodiscard]] virtual std::vector<DiskSample> readDisks() = 0;
};

} // namespace systemlens::providers
