/**
 * @file    WindowsStartupProvider.h
 * @brief   Windows IStartupProvider: registry Run keys and the Startup folders.
 */
#pragma once

#include "providers/IStartupProvider.h"

namespace systemlens::providers::windows_os {

class WindowsStartupProvider final : public IStartupProvider
{
public:
    [[nodiscard]] std::vector<StartupItem> readStartupItems() const override;
};

} // namespace systemlens::providers::windows_os
