/**
 * @file    LinuxMemoryProvider.h
 * @brief   Linux IMemoryProvider: /proc/meminfo.
 */
#pragma once

#include "providers/IMemoryProvider.h"

namespace systemlens::providers::linux_os {

class LinuxMemoryProvider final : public IMemoryProvider
{
public:
    [[nodiscard]] MemoryUsage readUsage() const override;
};

} // namespace systemlens::providers::linux_os
