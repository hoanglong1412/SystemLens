/**
 * @file    LinuxDiskProvider.cpp
 * @brief   Implementation of LinuxDiskProvider.
 */
#include "providers/linux/LinuxDiskProvider.h"

#include "providers/linux/ProcFs.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string_view>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kBlockDir = "/sys/block";
/** Optical drives have a "device" link too but are not disks. */
constexpr std::string_view kOpticalPrefix = "sr";
/** Index of io_ticks (milliseconds with I/O in flight) in /sys/block/<dev>/stat. */
constexpr std::size_t kIoTicksIndex = 9;
constexpr std::size_t kSectorsReadIndex = 2;
constexpr std::size_t kSectorsWrittenIndex = 6;
/** The kernel counts sectors of 512 bytes in stat and size, whatever the real sector size is. */
constexpr std::uint64_t kBytesPerSector = 512;

/**
 * @brief Tell SSD from HDD using queue/rotational.
 * @param blockPath /sys/block/<dev>.
 * @return Ssd, Hdd, or Unknown when the kernel does not say.
 */
DiskKind readKind(const std::filesystem::path& blockPath)
{
    const auto rotational = procfs::readUnsigned(blockPath / "queue" / "rotational");
    if (!rotational)
    {
        return DiskKind::Unknown;
    }
    return *rotational == 0 ? DiskKind::Ssd : DiskKind::Hdd;
}

/** Raw counters of /sys/block/<dev>/stat that the provider uses. */
struct DiskStat
{
    std::uint64_t sectorsRead = 0;
    std::uint64_t sectorsWritten = 0;
    std::uint64_t ioTicksMs = 0; ///< Milliseconds with I/O in flight.
};

/**
 * @brief Read the counters of a block device.
 * @param blockPath /sys/block/<dev>.
 * @return The counters, or std::nullopt when the stat file is unreadable or too short.
 */
std::optional<DiskStat> readStat(const std::filesystem::path& blockPath)
{
    const auto content = procfs::readFile(blockPath / "stat");
    if (!content)
    {
        return std::nullopt;
    }
    const auto fields = procfs::splitWhitespace(*content);
    if (fields.size() <= kIoTicksIndex)
    {
        return std::nullopt;
    }
    const auto read = procfs::parseUnsigned(fields[kSectorsReadIndex]);
    const auto written = procfs::parseUnsigned(fields[kSectorsWrittenIndex]);
    const auto ticks = procfs::parseUnsigned(fields[kIoTicksIndex]);
    if (!read || !written || !ticks)
    {
        return std::nullopt;
    }
    return DiskStat{.sectorsRead = *read, .sectorsWritten = *written, .ioTicksMs = *ticks};
}

} // namespace

/**
 * @brief Enumerate physical disks: block devices backed by a device (no loop, ram, dm, zram).
 * @return Disks sorted by kernel name and numbered in that order; empty when /sys/block is unreadable.
 */
std::vector<DiskSample> LinuxDiskProvider::readDisks()
{
    std::vector<std::string> names;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(kBlockDir, error))
    {
        const std::string name = entry.path().filename().string();
        // Virtual block devices (loop, ram, dm-*, zram) have no "device" link.
        if (!name.starts_with(kOpticalPrefix) && std::filesystem::exists(entry.path() / "device", error))
        {
            names.push_back(name);
        }
    }
    std::ranges::sort(names);

    std::vector<DiskSample> disks;
    disks.reserve(names.size());
    for (std::size_t index = 0; index < names.size(); ++index)
    {
        const std::filesystem::path blockPath = std::filesystem::path(kBlockDir) / names[index];
        DiskSample disk;
        disk.id = "disk" + std::to_string(index);
        disk.name = "Disk " + std::to_string(index) + " (" + names[index] + ")";
        disk.kind = readKind(blockPath);
        disk.model = procfs::readTrimmed(blockPath / "device" / "model").value_or("");
        if (const auto sectors = procfs::readUnsigned(blockPath / "size"))
        {
            disk.capacityBytes = *sectors * kBytesPerSector;
        }
        if (const auto stat = readStat(blockPath))
        {
            disk.busyTime = std::chrono::duration<double, std::milli>(static_cast<double>(stat->ioTicksMs));
            disk.bytesRead = stat->sectorsRead * kBytesPerSector;
            disk.bytesWritten = stat->sectorsWritten * kBytesPerSector;
        }
        disks.push_back(std::move(disk));
    }
    return disks;
}

} // namespace systemlens::providers::linux_os
