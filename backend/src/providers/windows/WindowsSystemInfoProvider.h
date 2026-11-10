/**
 * @file    WindowsSystemInfoProvider.h
 * @brief   Windows ISystemInfoProvider: registry, GetPerformanceInfo, Service Control Manager, WTS.
 */
#pragma once

#include "providers/ISystemInfoProvider.h"

namespace systemlens::providers::windows_os {

class WindowsSystemInfoProvider final : public ISystemInfoProvider
{
public:
    [[nodiscard]] SystemIdentity readIdentity() const override;
    [[nodiscard]] std::chrono::system_clock::time_point readBootTime() const override;
    [[nodiscard]] SystemCounts readCounts() const override;
};

} // namespace systemlens::providers::windows_os
