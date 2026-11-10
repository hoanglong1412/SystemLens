/**
 * @file    LinuxDiskProvider.cpp
 * @brief   Implementation of LinuxDiskProvider.
 */
#include "providers/linux/LinuxDiskProvider.h"

#include "providers/linux/ProcFs.h"

#include <algorithm>
#include <filesystem>
#include <string_view>

namespace systemlens::providers::linux_os {

namespace {

constexpr const char* kBlockDir = "/sys/block";
/** Optical drives have a "device" link too but are not disks. */
constexpr std::string_view kOpticalPrefix = "sr";
/** Index of io_ticks (milliseconds with I/O in flight) in /sys/block/<dev>/stat. */
constexpr std::size_t kIoTicksIndex = 9;

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

/**
 * @brief Read the cumulative busy time of a block device.
 * @param blockPath /sys/block/<dev>.
 * @return io_ticks as a duration, or std::nullopt when the stat file is unreadable.
 */
std::optional<std::chrono::duration<double>> readBusyTime(const std::filesystem::path& blockPath)
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
    const auto milliseconds = procfs::parseUnsigned(fields[kIoTicksIndex]);
    if (!milliseconds)
    {
        return std::nullopt;
    }
    return std::chrono::duration<double, std::milli>(static_cast<double>(*milliseconds));
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
        disks.push_back(DiskSample{
            .id = "disk" + std::to_string(index),
            .name = "Disk " + std::to_string(index) + " (" + names[index] + ")",
            .kind = readKind(blockPath),
            .busyTime = readBusyTime(blockPath),
        });
    }
    return disks;
}

} // namespace systemlens::providers::linux_os
