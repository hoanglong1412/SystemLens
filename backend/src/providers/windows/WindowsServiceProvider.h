/**
 * @file    WindowsServiceProvider.h
 * @brief   Windows ISystemServiceProvider: Service Control Manager.
 */
#pragma once

#include "providers/ISystemServiceProvider.h"

namespace systemlens::providers::windows_os {

class WindowsServiceProvider final : public ISystemServiceProvider
{
public:
    [[nodiscard]] std::vector<SystemServiceInfo> readServices() const override;
};

} // namespace systemlens::providers::windows_os
