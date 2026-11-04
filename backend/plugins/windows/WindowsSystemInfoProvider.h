/**
 * @file    WindowsSystemInfoProvider.h
 * @brief   ISystemInfoProvider of the Windows platform plugin: computer name, Windows edition and
 *          build, user name.
 */
#pragma once

#include "system/ISystemInfoProvider.h"

namespace systemlens::plugin {

class WindowsSystemInfoProvider final : public providers::system::ISystemInfoProvider
{
public:
    providers::system::SystemIdentity getIdentity() const override;
};

} // namespace systemlens::plugin
