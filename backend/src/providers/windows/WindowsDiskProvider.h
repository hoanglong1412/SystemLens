/**
 * @file    WindowsDiskProvider.h
 * @brief   Windows IDiskProvider: IOCTLs on the PhysicalDriveN devices.
 */
#pragma once

#include "providers/IDiskProvider.h"

namespace systemlens::providers::windows_os {

class WindowsDiskProvider final : public IDiskProvider
{
public:
    [[nodiscard]] std::vector<DiskSample> readDisks() override;
};

} // namespace systemlens::providers::windows_os
