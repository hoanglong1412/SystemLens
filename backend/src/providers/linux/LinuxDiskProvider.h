/**
 * @file    LinuxDiskProvider.h
 * @brief   Linux IDiskProvider: /sys/block.
 */
#pragma once

#include "providers/IDiskProvider.h"

namespace systemlens::providers::linux_os {

class LinuxDiskProvider final : public IDiskProvider
{
public:
    [[nodiscard]] std::vector<DiskSample> readDisks() override;
};

} // namespace systemlens::providers::linux_os
