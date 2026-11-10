/**
 * @file    LinuxSystemInfoProvider.h
 * @brief   Linux ISystemInfoProvider: /etc/os-release, uname, /proc/stat, /proc/loadavg.
 */
#pragma once

#include "providers/ISystemInfoProvider.h"

namespace systemlens::providers::linux_os {

class LinuxSystemInfoProvider final : public ISystemInfoProvider
{
public:
    [[nodiscard]] SystemIdentity readIdentity() const override;
    [[nodiscard]] std::chrono::system_clock::time_point readBootTime() const override;
    [[nodiscard]] SystemCounts readCounts() const override;
};

} // namespace systemlens::providers::linux_os
