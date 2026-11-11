/**
 * @file    LinuxServiceProvider.h
 * @brief   Linux ISystemServiceProvider: systemd units, read through the systemctl command.
 */
#pragma once

#include "providers/ISystemServiceProvider.h"

namespace systemlens::providers::linux_os {

class LinuxServiceProvider final : public ISystemServiceProvider
{
public:
    [[nodiscard]] std::vector<SystemServiceInfo> readServices() const override;
};

} // namespace systemlens::providers::linux_os
