/**
 * @file    LinuxGpuProvider.h
 * @brief   Linux IGpuProvider: /sys/class/drm (utilization only where the driver exports it).
 */
#pragma once

#include "providers/IGpuProvider.h"

#include <filesystem>

namespace systemlens::providers::linux_os {

class LinuxGpuProvider final : public IGpuProvider
{
public:
    LinuxGpuProvider();
    [[nodiscard]] std::vector<GpuSample> readGpus() override;

private:
    /** A graphics card found at construction; cards do not change while the backend runs. */
    struct Card
    {
        std::filesystem::path devicePath; ///< /sys/class/drm/cardN/device
        std::string model;
    };

    std::vector<Card> m_cards;
};

} // namespace systemlens::providers::linux_os
