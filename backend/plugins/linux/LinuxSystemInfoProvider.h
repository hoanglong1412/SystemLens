/**
 * @file    LinuxSystemInfoProvider.h
 * @brief   ISystemInfoProvider of the Linux platform plugin: uname, /etc/os-release, passwd database.
 */
#pragma once

#include "system/ISystemInfoProvider.h"

namespace systemlens::plugin {

class LinuxSystemInfoProvider final : public providers::system::ISystemInfoProvider
{
public:
    providers::system::SystemIdentity getIdentity() const override;
};

} // namespace systemlens::plugin
