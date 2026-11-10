/**
 * @file    LinuxCpuProvider.h
 * @brief   Linux ICpuProvider: /proc/stat, /proc/cpuinfo, cpufreq and hwmon in /sys.
 */
#pragma once

#include "providers/ICpuProvider.h"

#include <filesystem>
#include <optional>

namespace systemlens::providers::linux_os {

class LinuxCpuProvider final : public ICpuProvider
{
public:
    LinuxCpuProvider();
    [[nodiscard]] CpuStaticInfo readStaticInfo() const override;
    [[nodiscard]] CpuSample readSample() override;

private:
    double m_secondsPerTick; ///< Length of one clock tick of /proc/stat.
    /** Temperature sensor file found at construction; nullopt when the machine exposes none. */
    std::optional<std::filesystem::path> m_temperatureFile;
};

} // namespace systemlens::providers::linux_os
