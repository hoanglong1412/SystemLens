/**
 * @file    LinuxStartupProvider.h
 * @brief   Linux IStartupProvider: XDG autostart .desktop files.
 */
#pragma once

#include "providers/IStartupProvider.h"

namespace systemlens::providers::linux_os {

class LinuxStartupProvider final : public IStartupProvider
{
public:
    [[nodiscard]] std::vector<StartupItem> readStartupItems() const override;
};

} // namespace systemlens::providers::linux_os
