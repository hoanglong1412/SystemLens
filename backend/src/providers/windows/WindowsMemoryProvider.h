/**
 * @file    WindowsMemoryProvider.h
 * @brief   Windows IMemoryProvider: GlobalMemoryStatusEx.
 */
#pragma once

#include "providers/IMemoryProvider.h"

namespace systemlens::providers::windows_os {

class WindowsMemoryProvider final : public IMemoryProvider
{
public:
    [[nodiscard]] MemoryUsage readUsage() const override;
};

} // namespace systemlens::providers::windows_os
